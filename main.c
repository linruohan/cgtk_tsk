#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <libadwaita-1/adwaita.h>
#include <stdio.h>

static const char *const DEF_BIN_LABELS[] = {
    "(", ")", NULL, "/",
    "9", "8", "7", "*",
    "6", "5", "4", "-",
    "3", "2", "1", "+",
    ".", "0", "=",
};
static const int DEF_BIN_LABEL_LEN = G_N_ELEMENTS(DEF_BIN_LABELS) - 1;

typedef struct _TExprItem {
    bool ifOp;
    char *op;
    double value;
} TExprItem;

// 操作数特化的构造函数
TExprItem *t_expr_item_new_double(double value) {
    TExprItem *item = g_malloc0(sizeof(TExprItem));
    item->value = value;
    return item;
}

//操作符特化的构造函数
TExprItem *t_expr_item_new_pointer(const char *op) {
    TExprItem *item = g_malloc0(sizeof(TExprItem));
    item->ifOp = true;
    item->op = g_strdup(op);
    return item;
}


TExprItem *t_expr_item_new(GType type, ...) {
    va_list args;
    va_start(args, type);
    TExprItem *item = nullptr;
    switch (type) {
        case G_TYPE_DOUBLE:
            item = t_expr_item_new_double(va_arg(args, double));
            break;
        case G_TYPE_POINTER:
            item = t_expr_item_new_pointer(va_arg(args, const char*));
            break;
        default: break;
    }
    va_end(args);
    return item;
}


void t_expr_item_free(TExprItem *item) {
    if (item->ifOp)
        g_free(item->op);
    g_free(item);
};

//判断字符是否为操作符
bool comm_is_operator(char ch) {
    return ch == '+' || ch == '-' || ch == '*' || ch == '/';
}

//获取操作符优先级
int common_precedence(const char ch) {
    switch (ch) {
        case '+':
        case '-':
            return 1;
        case '*':
        case '/':
            return 2;
        default:
            return 0;
    }
}

static GScannerConfig *_scanner_config_new() {
    static const char def_skip_characters[] = " \t\n\r\f";
    static const char def_identifier_first[] = "+-*/()";
    GScannerConfig *config = g_malloc0(sizeof(GScannerConfig));
    config->cset_skip_characters = g_strdup(def_skip_characters); //跳过的特殊字符
    config->cset_identifier_first = g_strdup(def_identifier_first); //标识符的第一个字符
    config->scan_identifier = true; //扫描标识符
    config->scan_identifier_1char = true; //标识符只有1个字符
    config->scan_float = true; //扫描浮点数
    config->numbers_2_int = true; //转int
    config->int_2_float = true; //转浮点
    return config;
};

GScannerConfig *_scanner_config_free(GScannerConfig *config) {
    g_free(config->cset_skip_characters);
    g_free(config->cset_identifier_first);
    g_free(config);
};
//转换后缀表达式
GQueue *comm_infix_to_postfix(const char *infix) {
    const GScannerConfig *config = _scanner_config_new();
    GScanner *scanner = g_scanner_new(config);
    //操作数和操作符
    GQueue *ops = g_queue_new(), *res = g_queue_new();
    TExprItem *exp = NULL;
    g_scanner_input_text(scanner, infix, strlen(infix));
    while (G_TOKEN_EOF != g_scanner_get_next_token(scanner)) {
        if (G_TOKEN_FLOAT == scanner->token) {
            g_queue_push_head(res, t_expr_item_new(scanner->value.v_float));
        } else if (G_TOKEN_IDENTIFIER == scanner->token) {
            if ('(' == scanner->value.v_identifier[0])
                g_queue_push_head(ops, t_expr_item_new(G_TYPE_DOUBLE, scanner->value.v_identifier));
            else if (')' == scanner->value.v_identifier[0]) {
                exp = g_queue_pop_head(ops);
                if (exp->ifOp && '(' == exp->op[0]) {
                    t_expr_item_free(exp);
                    break;
                }
                g_queue_push_head(res, exp);
            } else if (comm_is_operator(scanner->value.v_identifier[0])) {
                while (!g_queue_is_empty(ops)) {
                    exp = g_queue_pop_head(ops);
                    if (exp->ifOp && common_precedence(exp->op[0]) < common_precedence(scanner->value.v_identifier[0]))
                        break;
                    g_queue_push_head(res, ops);
                }
                g_queue_push_head(ops, t_expr_item_new(G_TYPE_DOUBLE, scanner->value.v_identifier));
            }
        }
    };
    return res;
}

double common_evaluate_postfix(GQueue *postfix) {
    GQueue *stack = g_queue_new();
    double val = 0;
    char buff[1200] = {[0] = 0};
    while (!g_queue_is_empty(postfix)) {
        TExprItem *exp = g_queue_pop_tail(postfix);
        if (exp->ifOp) {
            strcat(buff, exp->op);
        } else {
            sprintf(buff, "%s %f ", buff, exp->value);
        }
        if (!exp->ifOp) {
            g_queue_push_head(stack, exp);
        } else {
            TExprItem *v1 = g_queue_pop_head(stack);
            TExprItem *v2 = g_queue_pop_head(stack);
            if (v1 && v2) {
                switch (*exp->op) {
                    case '+':
                        val = v1->value + v2->value;
                        break;
                    case '-':
                        val = v1->value - v2->value;
                        break;
                    case '*':
                        val = v1->value * v2->value;
                        break;
                    case '/':
                        val = v1->value / v2->value;
                        break;
                    default:
                        g_printf("%f ", exp->op[0]);
                        return 0;
                }
                g_queue_push_head(stack, t_expr_item_new(val));
                t_expr_item_free(v1);
                t_expr_item_free(v2);
            } else {
                g_printf("%f ", exp->value);
                return 0;
            }
        }
    }
    return val;
}

typedef enum _EAppWidget {
    E_WIDGET_LABEL_TEMP,
    E_WIDGET_ENTER_INPUT,
    E_WIDGET_BRACKET_LEFT,
    E_WIDGET_BTN_CLEAN,
    E_WIDGET_BTN_DIV,
    E_WIDGET_BTN9,
    E_WIDGET_BTN8,
    E_WIDGET_BTN7,
    E_WIDGET_MUL,
    E_WIDGET_BTM6,
    E_WIDGET_BTN5,
    E_WIDGET_BTN4,
    E_WIDGET_SUB,
    E_WIDGET_BTN3,
    E_WIDGET_BTN2,
    E_WIDGET_BTN1,
    E_WIDGET_ADD,
    E_WIDGET_DOT,
    E_WIDGET_BTN0,
    E_WIDGET_BTM_EQUAL, //指定小部件在TApp:widgets的下标
    E_WIDGET_CNT //数组的长度 。顺序从左到右，从上到下
} EAppWidget;

typedef struct _TApp {
    AdwApplication *app; //gtk应用程序实例
    GtkWidget **widgets; //所有小部件数组
} TApp;

TApp *t_app_new() {
    TApp *app = g_malloc0(sizeof(TApp));
    app->app = NULL;
    app->app = adw_application_new("com.github.linruohan", G_APPLICATION_DEFAULT_FLAGS);
    app->widgets = g_new0(GtkWidget*, E_WIDGET_CNT);
    return app;
};

void t_app_free(TApp *app) {
    g_object_unref(app->app);
    g_free(app->widgets);
    g_free(app);
}

void app_btn_operator_click(GtkButton *btn, TApp *usd) {
    const char *currText = gtk_editable_get_text(GTK_EDITABLE(usd->widgets[E_WIDGET_ENTER_INPUT]));
    char *newText = g_strdup_printf("%s%s", currText, gtk_button_get_label(btn));
    gtk_editable_set_text(GTK_EDITABLE(usd->widgets[E_WIDGET_ENTER_INPUT]), newText);
    g_free(newText);
}

void app_btn_equal_click(GtkButton *btn, TApp *usd) {
    const char *currText = gtk_editable_get_text(GTK_EDITABLE(usd->widgets[E_WIDGET_ENTER_INPUT]));
    GQueue *postfix = comm_infix_to_postfix(currText);
    double val = common_evaluate_postfix(postfix);
    char *res = g_strdup_printf("%f", val);
    gtk_label_set_text(GTK_LABEL(usd->widgets[E_WIDGET_LABEL_TEMP]), g_strdup_printf("%f", currText));
    gtk_editable_set_text(GTK_EDITABLE(usd->widgets[E_WIDGET_ENTER_INPUT]), g_strdup_printf("%f", res));
    g_queue_free_full(postfix, (GDestroyNotify) t_expr_item_free);
    g_free(res);
}

void app_btn_clean_click(GtkButton *btn, TApp *usd) {
    gtk_editable_set_text(GTK_EDITABLE(usd->widgets[E_WIDGET_ENTER_INPUT]), "");
}

static void app_activate(GtkApplication *app, TApp *usd) {
    GtkWidget *grid = gtk_grid_new();
    GtkWidget *label = usd->widgets[E_WIDGET_LABEL_TEMP] = gtk_label_new("结果：");
    GtkWidget *entry = usd->widgets[E_WIDGET_ENTER_INPUT] = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "请输入表达式");
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 4, 1);
    gtk_grid_attach(GTK_GRID(grid), entry, 0, 1, 4, 1);

    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
    gtk_grid_set_row_homogeneous(GTK_GRID(grid), TRUE);

    GtkWidget *btn;
    for (unsigned i = 0; i < DEF_BIN_LABEL_LEN; i++) {
        if (!DEF_BIN_LABELS[i]) continue;
        btn = usd->widgets[i + E_WIDGET_BRACKET_LEFT] = gtk_button_new_with_label(DEF_BIN_LABELS[i]);
        // 行是上面2行加下面的需要+2
        gtk_grid_attach(GTK_GRID(grid), btn, i % 4, i / 4 + 2, 1, 1);
        g_signal_connect(G_OBJECT(btn), "clicked", G_CALLBACK(app_btn_operator_click), usd);
    }

    //归零
    btn = usd->widgets[E_WIDGET_BTN_CLEAN] = gtk_button_new_with_label("C");
    gtk_grid_attach(GTK_GRID(grid), btn, 2, 2, 1, 1);
    g_signal_connect(btn, "clicked", G_CALLBACK(app_btn_clean_click), usd);

    //计算
    btn = usd->widgets[E_WIDGET_BTM_EQUAL] = gtk_button_new_with_label("=");
    gtk_grid_attach(GTK_GRID(grid), btn, 2, 6, 2, 1);
    g_signal_connect(btn, "clicked", G_CALLBACK(app_btn_equal_click), usd);

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "计算机示例");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 500);
    gtk_window_set_child(GTK_WINDOW(window), grid);
    gtk_window_present(GTK_WINDOW(window));
}

int
main(const int argc, char *argv[]) {
    TApp *app = t_app_new();
    g_signal_connect(app->app, "activate", G_CALLBACK (app_activate), app);
    const int rev = g_application_run(G_APPLICATION(app->app), argc, argv);
    t_app_free(app);
    return rev;
}

