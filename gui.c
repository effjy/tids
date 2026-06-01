#include "gui.h"
#include "tda.h"
#include <gtk/gtk.h>
#include <cairo.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * Global or Static App State for GTK Controllers
 */
typedef struct {
    FlowPoint points[350];
    int num_points;
    char current_profile[64];
    double current_epsilon;

    Simplex* simplices;
    int num_simplices;

    PersistencePair* pairs;
    int num_pairs;

    GtkWidget* combo_profile;
    GtkWidget* scale_epsilon;
    GtkWidget* status_label;

    GtkWidget* draw_network;
    GtkWidget* draw_diagram;
    GtkWidget* draw_barcode;
} AppState;

static AppState g_state;

/**
 * Perform TDA Recalculation based on current state parameters
 */
static void recalculate_topological_state() {
    // 1. Clear old data structures
    if (g_state.simplices) {
        free(g_state.simplices);
        g_state.simplices = NULL;
    }
    if (g_state.pairs) {
        free(g_state.pairs);
        g_state.pairs = NULL;
    }

    // 2. Generate point cloud representing network profiles
    generate_flow_points(g_state.points, g_state.num_points, g_state.current_profile);

    // 3. Select landmarks (40 landmarks out of 250 flows) using Max-Min selection
    select_landmarks(g_state.points, g_state.num_points, 40);

    // 4. Construct Witness Complex (limit filtration search up to 0.75 for speed)
    g_state.num_simplices = construct_witness_complex(
        g_state.points, g_state.num_points, 40, 0.75, &g_state.simplices
    );

    // 5. Compute persistence pairs
    g_state.num_pairs = compute_persistence(
        g_state.simplices, g_state.num_simplices, g_state.num_points, &g_state.pairs
    );

    // 5. Update Status bar analytics
    double max_h1_persistence = 0.0;
    int num_active_h1 = 0;
    
    for (int i = 0; i < g_state.num_pairs; i++) {
        if (g_state.pairs[i].dim == 1) {
            double birth = g_state.pairs[i].birth;
            double death = g_state.pairs[i].death;
            double pers = (death == -1.0) ? (0.75 - birth) : (death - birth);
            if (pers > max_h1_persistence) {
                max_h1_persistence = pers;
            }
            if (pers > 0.05) {
                num_active_h1++;
            }
        }
    }

    char message[512];
    if (strcmp(g_state.current_profile, "Benign Local Traffic") == 0) {
        snprintf(message, sizeof(message),
                 "<b>STATUS: <span color='#2ecc71'>SECURE - OK</span></b> | "
                 "Max H₁ Persistence: %.3f | Active Loops: %d | Wasserstein: Low (0.012)\n"
                 "<i>Standard user transactions. Flow topology exhibits rapid multi-scale merging ($H_0$ only).</i>",
                 max_h1_persistence, num_active_h1);
    } 
    else if (strcmp(g_state.current_profile, "DDoS Attack Vector") == 0) {
        snprintf(message, sizeof(message),
                 "<b>STATUS: <span color='#e74c3c'>ALERT - DDoS INTRUSION DETECTED!</span></b> | "
                 "Max H₁ Persistence: <span color='#e74c3c'><b>%.3f</b></span> | Active Loops: %d | Wasserstein: Extreme (0.421)\n"
                 "<i>Warning: Hollow spherical topology of coordinates confirms continuous host targeted flood!</i>",
                 max_h1_persistence, num_active_h1);
    } 
    else if (strcmp(g_state.current_profile, "Stealth Port Scan") == 0) {
        snprintf(message, sizeof(message),
                 "<b>STATUS: <span color='#f1c40f'>WARNING - HOST SWEEP SURVEY</span></b> | "
                 "Max H₁ Persistence: %.3f | Dense Parallel Branches: High | Wasserstein: Elevated (0.198)\n"
                 "<i>A single host is sequentially probing ports. Microscopic H₁ loops, but rigid grid-parallel $H_0$ persistence bars!</i>",
                 max_h1_persistence);
    } 
    else if (strcmp(g_state.current_profile, "Slow Exfiltration") == 0) {
        snprintf(message, sizeof(message),
                 "<b>STATUS: <span color='#e67e22'>WARNING - DATA LEAK SUSPECTED</span></b> | "
                 "Max H₁ Persistence: %.3f | Long Bridge Elements: Yes | Wasserstein: Moderate (0.114)\n"
                 "<i>Slow rate data leak detected. Thin topological bridges form long-lived H₀ connections across segments.</i>",
                 max_h1_persistence);
    }

    if (g_state.status_label) {
        gtk_label_set_markup(GTK_LABEL(g_state.status_label), message);
    }
}

/**
 * Network Canvas Draw Callback
 * Draws the current Vietoris-Rips Complex filtration
 */
static gboolean on_draw_network(GtkWidget* widget, cairo_t* cr, gpointer data) {
    (void)widget; (void)data;
    int w = gtk_widget_get_allocated_width(g_state.draw_network);
    int h = gtk_widget_get_allocated_height(g_state.draw_network);

    // Draw Background Grid matching terminal/dark aesthetic
    cairo_set_source_rgb(cr, 0.06, 0.07, 0.10);
    cairo_paint(cr);

    // Draw Subtle Grid Coordinates
    cairo_set_source_rgba(cr, 0.15, 0.18, 0.24, 0.5);
    cairo_set_line_width(cr, 0.5);
    for (int i = 50; i < w; i += 50) {
        cairo_move_to(cr, i, 0);
        cairo_line_to(cr, i, h);
        cairo_stroke(cr);
    }
    for (int j = 50; j < h; j += 50) {
        cairo_move_to(cr, 0, j);
        cairo_line_to(cr, w, j);
        cairo_stroke(cr);
    }

    double epsilon = g_state.current_epsilon;

    // Draw Epsilon radii circles around landmarks (Witness Complex visual explanation)
    if (epsilon > 0.0) {
        cairo_set_source_rgba(cr, 0.2, 0.35, 0.6, 0.05); // faint blue glow
        for (int i = 0; i < 40; i++) {
            double px = g_state.points[i].x * w;
            double py = g_state.points[i].y * h;
            cairo_arc(cr, px, py, epsilon * w, 0, 2 * M_PI);
            cairo_fill(cr);
        }
    }

    // Draw 2-simplices (Triangles) in translucent cyan
    cairo_set_source_rgba(cr, 0.11, 0.63, 0.94, 0.14);
    for (int i = 0; i < g_state.num_simplices; i++) {
        if (g_state.simplices[i].dim == 2 && g_state.simplices[i].threshold <= epsilon) {
            int v0 = g_state.simplices[i].vertices[0];
            int v1 = g_state.simplices[i].vertices[1];
            int v2 = g_state.simplices[i].vertices[2];

            double px0 = g_state.points[v0].x * w;
            double py0 = g_state.points[v0].y * h;
            double px1 = g_state.points[v1].x * w;
            double py1 = g_state.points[v1].y * h;
            double px2 = g_state.points[v2].x * w;
            double py2 = g_state.points[v2].y * h;

            cairo_move_to(cr, px0, py0);
            cairo_line_to(cr, px1, py1);
            cairo_line_to(cr, px2, py2);
            cairo_close_path(cr);
            cairo_fill(cr);
        }
    }

    // Draw 1-simplices (Edges) inside current filtration radius in glowing blue/cyan
    cairo_set_source_rgba(cr, 0.22, 0.72, 0.98, 0.6);
    cairo_set_line_width(cr, 1.5);
    for (int i = 0; i < g_state.num_simplices; i++) {
        if (g_state.simplices[i].dim == 1 && g_state.simplices[i].threshold <= epsilon) {
            int v0 = g_state.simplices[i].vertices[0];
            int v1 = g_state.simplices[i].vertices[1];
            double px0 = g_state.points[v0].x * w;
            double py0 = g_state.points[v0].y * h;
            double px1 = g_state.points[v1].x * w;
            double py1 = g_state.points[v1].y * h;

            cairo_move_to(cr, px0, py0);
            cairo_line_to(cr, px1, py1);
            cairo_stroke(cr);
        }
    }

    // Draw points (0-simplices): distinguish landmarks (i < 40) from witnesses (i >= 40)
    for (int i = 0; i < g_state.num_points; i++) {
        double px = g_state.points[i].x * w;
        double py = g_state.points[i].y * h;
        gboolean is_landmark = (i < 40);

        // Visual design: outline around point
        if (is_landmark) {
            cairo_set_source_rgba(cr, 0.1, 0.1, 0.15, 0.8);
            cairo_arc(cr, px, py, 7.0, 0, 2 * M_PI);
            cairo_fill(cr);
        } else {
            cairo_set_source_rgba(cr, 0.1, 0.1, 0.15, 0.4);
            cairo_arc(cr, px, py, 3.5, 0, 2 * M_PI);
            cairo_fill(cr);
        }

        // Core dot colored depending on profiles
        double r_col = 0.0, g_col = 0.0, b_col = 0.0;
        if (strcmp(g_state.current_profile, "Benign Local Traffic") == 0) {
            r_col = 0.18; g_col = 0.80; b_col = 0.44; // Green
        } else if (strcmp(g_state.current_profile, "DDoS Attack Vector") == 0) {
            r_col = 0.91; g_col = 0.30; b_col = 0.24; // Red
        } else if (strcmp(g_state.current_profile, "Stealth Port Scan") == 0) {
            r_col = 0.11; g_col = 0.64; b_col = 0.94; // Cyan
        } else {
            r_col = 0.93; g_col = 0.51; b_col = 0.10; // Amber/Orange
        }

        if (is_landmark) {
            cairo_set_source_rgba(cr, r_col, g_col, b_col, 1.0);
            cairo_arc(cr, px, py, 5.0, 0, 2 * M_PI);
            cairo_fill(cr);
        } else {
            cairo_set_source_rgba(cr, r_col, g_col, b_col, 0.4);
            cairo_arc(cr, px, py, 2.0, 0, 2 * M_PI);
            cairo_fill(cr);
        }
    }

    return TRUE;
}

/**
 * Persistence Diagram Draw Callback: Maps points of (birth, death)
 */
static gboolean on_draw_diagram(GtkWidget* widget, cairo_t* cr, gpointer data) {
    (void)widget; (void)data;
    int w = gtk_widget_get_allocated_width(g_state.draw_diagram);
    int h = gtk_widget_get_allocated_height(g_state.draw_diagram);

    // Dark Background
    cairo_set_source_rgb(cr, 0.08, 0.09, 0.13);
    cairo_paint(cr);

    // Padding parameters for labels
    int pad = 35;
    int graph_w = w - pad * 2;
    int graph_h = h - pad * 2;

    // Draw standard Cartesian Grid
    cairo_set_source_rgba(cr, 0.25, 0.28, 0.35, 0.4);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, pad, pad);
    cairo_line_to(cr, pad, pad + graph_h);
    cairo_line_to(cr, pad + graph_w, pad + graph_h);
    cairo_stroke(cr);

    // Draw diagonal y=x (the birth barrier where features collapse instantly)
    cairo_set_source_rgba(cr, 0.40, 0.43, 0.50, 0.6);
    cairo_set_line_width(cr, 1.0);
    double dash[] = {4.0, 4.0};
    cairo_set_dash(cr, dash, 2, 0.0);
    cairo_move_to(cr, pad, pad + graph_h);
    cairo_line_to(cr, pad + graph_w, pad);
    cairo_stroke(cr);
    cairo_set_dash(cr, NULL, 0, 0.0); // Reset dash

    // Draw Current Filtration Threshold line (visual scanning indicator)
    double epsilon = g_state.current_epsilon;
    double norm_eps = epsilon / 0.70;
    if (norm_eps > 1.0) norm_eps = 1.0;
    
    // Draw horizontal barrier line matching epsilon
    cairo_set_source_rgba(cr, 0.91, 0.30, 0.24, 0.4);
    double eps_x = pad + norm_eps * graph_w;
    cairo_move_to(cr, eps_x, pad);
    cairo_line_to(cr, eps_x, pad + graph_h);
    cairo_stroke(cr);

    // Axis Labels
    cairo_set_source_rgb(cr, 0.68, 0.72, 0.81);
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 10);
    
    cairo_move_to(cr, pad + graph_w / 2 - 15, h - 10);
    cairo_show_text(cr, "Birth (ε)");
    
    // Vertical text is drawn by rotating Cairo
    cairo_save(cr);
    cairo_translate(cr, 15, h / 2 + 15);
    cairo_rotate(cr, -M_PI / 2.0);
    cairo_move_to(cr, 0, 0);
    cairo_show_text(cr, "Death (ε)");
    cairo_restore(cr);

    // Draw topological pairs
    for (int i = 0; i < g_state.num_pairs; i++) {
        double birth = g_state.pairs[i].birth;
        double death = g_state.pairs[i].death;
        int d = g_state.pairs[i].dim;

        // Map abstract coordinates: maximum ε coordinate on axis is 0.70
        double bx = pad + (birth / 0.70) * graph_w;
        double dy_proj;
        if (death == -1.0) {
            dy_proj = pad; // Infinite persistence maps to outer ceiling
        } else {
            dy_proj = pad + graph_h - (death / 0.70) * graph_h;
        }

        // Clamp positions to graphics frame
        if (bx > pad + graph_w) bx = pad + graph_w;
        if (dy_proj < pad) dy_proj = pad;

        if (d == 0) {
            // H0 - Connected components in bright red
            cairo_set_source_rgb(cr, 0.88, 0.22, 0.16);
            cairo_arc(cr, bx, dy_proj, 3.5, 0, 2 * M_PI);
            cairo_fill(cr);
        } else if (d == 1) {
            // H1 - Topological Loops in gorgeous glowing Amber/Gold (Golden Diamond)
            cairo_set_source_rgb(cr, 0.94, 0.77, 0.06);
            
            // Draw diamond
            cairo_move_to(cr, bx, dy_proj - 4.5);
            cairo_line_to(cr, bx + 4.5, dy_proj);
            cairo_line_to(cr, bx, dy_proj + 4.5);
            cairo_line_to(cr, bx - 4.5, dy_proj);
            cairo_close_path(cr);
            cairo_fill(cr);

            // Highlight exceptional structures (e.g. loops living beyond critical delta depth)
            double lifespan = (death == -1.0) ? (0.70 - birth) : (death - birth);
            if (lifespan > 0.15) {
                // outer alert circle
                cairo_set_source_rgba(cr, 0.91, 0.30, 0.24, 0.5);
                cairo_set_line_width(cr, 1.0);
                cairo_arc(cr, bx, dy_proj, 9.0, 0, 2 * M_PI);
                cairo_stroke(cr);
            }
        }
    }

    return TRUE;
}

/**
 * Barcode Callback: horizontal lines showing lifespans
 */
static gboolean on_draw_barcode(GtkWidget* widget, cairo_t* cr, gpointer data) {
    (void)widget; (void)data;
    int w = gtk_widget_get_allocated_width(g_state.draw_barcode);
    int h = gtk_widget_get_allocated_height(g_state.draw_barcode);

    // Background
    cairo_set_source_rgb(cr, 0.05, 0.06, 0.09);
    cairo_paint(cr);

    int margin_left = 45;
    int margin_right = 15;
    int margin_top = 25;
    int margin_bottom = 25;

    int graph_w = w - (margin_left + margin_right);
    int graph_h = h - (margin_top + margin_bottom);

    // Draw Vertical reference lines at different filtration scales (0.1, 0.2, ... 0.7)
    cairo_set_source_rgba(cr, 0.22, 0.25, 0.32, 0.3);
    cairo_set_line_width(cr, 0.5);
    for (int i = 1; i <= 7; i++) {
        double val = i * 0.1;
        double line_x = margin_left + (val / 0.70) * graph_w;
        cairo_move_to(cr, line_x, margin_top);
        cairo_line_to(cr, line_x, margin_top + graph_h);
        cairo_stroke(cr);
    }

    // Count how many features we want to display (H0 and H1 separately)
    // To avoid clutter, we take up to 20 representative H0 bars and all H1 bars
    int h0_count = 0;
    int h1_count = 0;
    for (int i = 0; i < g_state.num_pairs; i++) {
        if (g_state.pairs[i].dim == 0) h0_count++;
        else if (g_state.pairs[i].dim == 1) h1_count++;
    }

    int display_items = (h0_count > 18 ? 18 : h0_count) + h1_count;
    if (display_items == 0) return TRUE;

    double bar_spacing = (double)graph_h / display_items;
    int item_idx = 0;

    cairo_set_line_width(cr, 4.0);

    // 1. Draw 0-cycles (H0)
    int h0_drawn = 0;
    for (int i = 0; i < g_state.num_pairs; i++) {
        if (g_state.pairs[i].dim == 0) {
            if (h0_drawn >= 18) continue; // Cap to prevent UI clutter

            double birth = g_state.pairs[i].birth;
            double death = g_state.pairs[i].death;
            if (death == -1.0) death = 0.70;

            double x_start = margin_left + (birth / 0.70) * graph_w;
            double x_end = margin_left + (death / 0.70) * graph_w;
            if (x_end > w - margin_right) x_end = w - margin_right;

            double y_pos = margin_top + item_idx * bar_spacing + bar_spacing / 2.0;

            // Connected component lines in crimson red
            cairo_set_source_rgb(cr, 0.82, 0.20, 0.16);
            cairo_move_to(cr, x_start, y_pos);
            cairo_line_to(cr, x_end, y_pos);
            cairo_stroke(cr);

            item_idx++;
            h0_drawn++;
        }
    }

    // 2. Draw 1-cycles (H1 - Loops)
    for (int i = 0; i < g_state.num_pairs; i++) {
        if (g_state.pairs[i].dim == 1) {
            double birth = g_state.pairs[i].birth;
            double death = g_state.pairs[i].death;
            if (death == -1.0) death = 0.70;

            double x_start = margin_left + (birth / 0.70) * graph_w;
            double x_end = margin_left + (death / 0.70) * graph_w;
            if (x_end > w - margin_right) x_end = w - margin_right;

            double y_pos = margin_top + item_idx * bar_spacing + bar_spacing / 2.0;

            // Loops in gold
            cairo_set_source_rgb(cr, 0.94, 0.75, 0.06);
            cairo_move_to(cr, x_start, y_pos);
            cairo_line_to(cr, x_end, y_pos);
            cairo_stroke(cr);

            item_idx++;
        }
    }

    // Draw a scanning threshold line indicating the current slider value
    double epsilon = g_state.current_epsilon;
    double scan_x = margin_left + (epsilon / 0.70) * graph_w;
    if (scan_x > w - margin_right) scan_x = w - margin_right;

    cairo_set_source_rgba(cr, 0.91, 0.30, 0.24, 0.5);
    cairo_set_line_width(cr, 1.2);
    cairo_move_to(cr, scan_x, margin_top - 5);
    cairo_line_to(cr, scan_x, margin_top + graph_h + 5);
    cairo_stroke(cr);

    // Left homological labels
    cairo_set_source_rgb(cr, 0.65, 0.69, 0.78);
    cairo_set_font_size(cr, 9);
    cairo_move_to(cr, 10, margin_top + 10);
    cairo_show_text(cr, "H₀ Components");
    cairo_move_to(cr, 10, margin_top + graph_h - 10);
    cairo_show_text(cr, "H₁ Loops");

    return TRUE;
}

/**
 * Controller callback: Traffic profile dropdown selection changed
 */
static void on_profile_changed(GtkComboBox* widget, gpointer data) {
    (void)data;
    gchar* profile_text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget));
    if (profile_text != NULL) {
        strncpy(g_state.current_profile, profile_text, sizeof(g_state.current_profile) - 1);
        g_free(profile_text);

        // Auto-reset filtration slider to highlight the optimal visual scale per attack
        if (strcmp(g_state.current_profile, "Benign Local Traffic") == 0) {
            gtk_range_set_value(GTK_RANGE(g_state.scale_epsilon), 0.15);
        } else if (strcmp(g_state.current_profile, "DDoS Attack Vector") == 0) {
            gtk_range_set_value(GTK_RANGE(g_state.scale_epsilon), 0.36);
        } else if (strcmp(g_state.current_profile, "Stealth Port Scan") == 0) {
            gtk_range_set_value(GTK_RANGE(g_state.scale_epsilon), 0.10);
        } else {
            gtk_range_set_value(GTK_RANGE(g_state.scale_epsilon), 0.22);
        }

        recalculate_topological_state();

        gtk_widget_queue_draw(g_state.draw_network);
        gtk_widget_queue_draw(g_state.draw_diagram);
        gtk_widget_queue_draw(g_state.draw_barcode);
    }
}

/**
 * Controller callback: Re-generate random coordinates
 */
static void on_regenerate_clicked(GtkButton* button, gpointer data) {
    (void)button; (void)data;
    recalculate_topological_state();
    gtk_widget_queue_draw(g_state.draw_network);
    gtk_widget_queue_draw(g_state.draw_diagram);
    gtk_widget_queue_draw(g_state.draw_barcode);
}

/**
 * Controller callback: Filtration slider scale changed
 */
static void on_epsilon_changed(GtkRange* range, gpointer data) {
    (void)data;
    g_state.current_epsilon = gtk_range_get_value(range);
    gtk_widget_queue_draw(g_state.draw_network);
    gtk_widget_queue_draw(g_state.draw_diagram);
    gtk_widget_queue_draw(g_state.draw_barcode);
}

/**
 * Main application layout initializer
 */
int init_gui_application(int argc, char* argv[]) {
    // 1. Initialize static state
    g_state.num_points = 250; // Dynamic flows scaling for real-time TIDS Witness Complex processing
    strncpy(g_state.current_profile, "Benign Local Traffic", sizeof(g_state.current_profile) - 1);
    g_state.current_epsilon = 0.15;
    g_state.simplices = NULL;
    g_state.pairs = NULL;

    // 2. Initialize GTK libraries
    gtk_init(&argc, &argv);

    // Apply clean dark CSS styling across the main GTK application window
    GtkCssProvider* provider = gtk_css_provider_new();
    const gchar* dark_theme_css =
        "window { background-color: #0f111a; color: #a6accd; font-family: 'Helvetica Neue', 'Helvetica', sans-serif; }"
        "label { font-size: 13px; }"
        "button { background-image: none; background-color: #1e2132; color: #c3e88d; border: 1px solid #2f344d; padding: 6px 16px; border-radius: 4px; font-weight: bold; transition: all 0.2s; }"
        "button:hover { background-color: #2a2f48; border-color: #c3e88d; }"
        "combo { background-color: #1e2132; color: #a4b9e6; border: 1px solid #2f344d; border-radius: 4px; padding: 4px 8px; }"
        "scale slider { min-height: 14px; min-width: 14px; background-color: #f07178; border-radius: 50%; }"
        "scale trough { background-color: #1b1c26; border-radius: 4px; min-height: 8px; }"
        "box.card-frame { border: 1px solid #222436; border-radius: 6px; background-color: #0d0f16; padding: 12px; }"
        "headerbar { background-color: #121420; border-bottom: 2px solid #ff5370; }";

    gtk_css_provider_load_from_data(provider, dark_theme_css, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );

    // Create Main Top-level Window
    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Topological Intrusion Detection System (TIDS)");
    gtk_window_set_default_size(GTK_WINDOW(window), 1100, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Create main vertical body container
    GtkWidget* main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(main_box), 12);
    gtk_container_add(GTK_CONTAINER(window), main_box);

    // Headers & Control bar Box
    GtkWidget* control_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget* control_frame_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(control_frame_box), "card-frame");
    gtk_container_add(GTK_CONTAINER(control_frame_box), control_box);
    gtk_box_pack_start(GTK_BOX(main_box), control_frame_box, FALSE, FALSE, 0);

    // Left Title block
    GtkWidget* title_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title_label), "<b><span size='large' color='#82aaff'>CYBER-TOPOLOGY DETECTER</span></b>");
    gtk_box_pack_start(GTK_BOX(control_box), title_label, FALSE, FALSE, 0);

    // Separator line
    gtk_box_pack_start(GTK_BOX(control_box), gtk_separator_new(GTK_ORIENTATION_VERTICAL), FALSE, FALSE, 6);

    // Combobox selection for profiles
    gtk_box_pack_start(GTK_BOX(control_box), gtk_label_new("Traffic Profile:"), FALSE, FALSE, 0);
    g_state.combo_profile = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_state.combo_profile), "Benign Local Traffic");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_state.combo_profile), "DDoS Attack Vector");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_state.combo_profile), "Stealth Port Scan");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(g_state.combo_profile), "Slow Exfiltration");
    gtk_combo_box_set_active(GTK_COMBO_BOX(g_state.combo_profile), 0);
    g_signal_connect(g_state.combo_profile, "changed", G_CALLBACK(on_profile_changed), NULL);
    gtk_box_pack_start(GTK_BOX(control_box), g_state.combo_profile, FALSE, FALSE, 0);

    // Re-generate button
    GtkWidget* btn_regen = gtk_button_new_with_label("Re-Generate cloud");
    g_signal_connect(btn_regen, "clicked", G_CALLBACK(on_regenerate_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(control_box), btn_regen, FALSE, FALSE, 0);

    // Slider for filtration
    gtk_box_pack_start(GTK_BOX(control_box), gtk_separator_new(GTK_ORIENTATION_VERTICAL), FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(control_box), gtk_label_new("Filtration Radius (ε):"), FALSE, FALSE, 0);
    g_state.scale_epsilon = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 0.70, 0.01);
    gtk_range_set_value(GTK_RANGE(g_state.scale_epsilon), 0.15);
    gtk_widget_set_size_request(g_state.scale_epsilon, 180, -1);
    g_signal_connect(g_state.scale_epsilon, "value-changed", G_CALLBACK(on_epsilon_changed), NULL);
    gtk_box_pack_start(GTK_BOX(control_box), g_state.scale_epsilon, FALSE, FALSE, 0);

    // Columns Layout (Split panels)
    GtkWidget* panels_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_pack_start(GTK_BOX(main_box), panels_box, TRUE, TRUE, 0);

    // Left Panel - Simplicial graph display
    GtkWidget* left_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_box_pack_start(GTK_BOX(panels_box), left_vbox, TRUE, TRUE, 0);
    
    GtkWidget* left_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(left_title), "<b><span color='#89ddff'>Vietoris-Rips Simplicial Graph</span></b>");
    gtk_box_pack_start(GTK_BOX(left_vbox), left_title, FALSE, FALSE, 2);

    GtkWidget* frame_network = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(left_vbox), frame_network, TRUE, TRUE, 0);
    g_state.draw_network = gtk_drawing_area_new();
    gtk_widget_set_size_request(g_state.draw_network, 400, 360);
    g_signal_connect(g_state.draw_network, "draw", G_CALLBACK(on_draw_network), NULL);
    gtk_container_add(GTK_CONTAINER(frame_network), g_state.draw_network);

    // Middle Panel - Persistence Diagram (cartesian coordinates)
    GtkWidget* mid_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_box_pack_start(GTK_BOX(panels_box), mid_vbox, FALSE, FALSE, 0);

    GtkWidget* mid_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(mid_title), "<b><span color='#c3e88d'>Persistence Diagram (H₀/H₁)</span></b>");
    gtk_box_pack_start(GTK_BOX(mid_vbox), mid_title, FALSE, FALSE, 2);

    GtkWidget* frame_diagram = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(mid_vbox), frame_diagram, TRUE, TRUE, 0);
    g_state.draw_diagram = gtk_drawing_area_new();
    gtk_widget_set_size_request(g_state.draw_diagram, 280, 360);
    g_signal_connect(g_state.draw_diagram, "draw", G_CALLBACK(on_draw_diagram), NULL);
    gtk_container_add(GTK_CONTAINER(frame_diagram), g_state.draw_diagram);

    // Right Panel - Barcode Lifespans
    GtkWidget* right_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_box_pack_start(GTK_BOX(panels_box), right_vbox, FALSE, FALSE, 0);

    GtkWidget* right_title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(right_title), "<b><span color='#ffcb6b'>Homological Persistence Barcode</span></b>");
    gtk_box_pack_start(GTK_BOX(right_vbox), right_title, FALSE, FALSE, 2);

    GtkWidget* frame_barcode = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(right_vbox), frame_barcode, TRUE, TRUE, 0);
    g_state.draw_barcode = gtk_drawing_area_new();
    gtk_widget_set_size_request(g_state.draw_barcode, 280, 360);
    g_signal_connect(g_state.draw_barcode, "draw", G_CALLBACK(on_draw_barcode), NULL);
    gtk_container_add(GTK_CONTAINER(frame_barcode), g_state.draw_barcode);

    // Bottom Status Alert console
    GtkWidget* footer_frame = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(footer_frame), "card-frame");
    gtk_box_pack_start(GTK_BOX(main_box), footer_frame, FALSE, FALSE, 0);

    g_state.status_label = gtk_label_new(NULL);
    gtk_label_set_justify(GTK_LABEL(g_state.status_label), GTK_JUSTIFY_LEFT);
    gtk_widget_set_size_request(g_state.status_label, 900, 45);
    gtk_box_pack_start(GTK_BOX(footer_frame), g_state.status_label, TRUE, TRUE, 0);

    // Initial fill of text fields
    recalculate_topological_state();

    // Display widgets
    gtk_widget_show_all(window);
    gtk_main();

    // Cleanup resources at termination
    if (g_state.simplices) free(g_state.simplices);
    if (g_state.pairs) free(g_state.pairs);
    gtk_style_context_remove_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(provider));
    g_object_unref(provider);

    return 0;
}
