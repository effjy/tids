#include "tda.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Linear Congruential Generator for portable deterministic randomness
static unsigned int rand_seed = 123456789;
static double tda_rand() {
    rand_seed = rand_seed * 1103515245 + 12345;
    return (double)(rand_seed / 65536 % 32768) / 32768.0;
}

void generate_flow_points(FlowPoint* points, int n, const char* profile) {
    rand_seed = 987654321; // Reset seed for consistent visual profiles
    
    if (strcmp(profile, "Benign Local Traffic") == 0) {
        // Create 3 dense localized clusters of client-to-service flows
        double centers_x[3] = {0.25, 0.70, 0.50};
        double centers_y[3] = {0.30, 0.40, 0.75};
        double centers_t[3] = {0.20, 0.50, 0.80};
        
        for (int i = 0; i < n; i++) {
            int c = i % 3;
            double radius = 0.05 + 0.05 * tda_rand();
            double angle = tda_rand() * 2.0 * M_PI;
            
            points[i].x = centers_x[c] + radius * cos(angle);
            points[i].y = centers_y[c] + radius * sin(angle);
            
            // Map abstract properties
            points[i].t = centers_t[c] + 0.06 * (tda_rand() - 0.5);
            points[i].dt = 0.02 + 0.03 * tda_rand();
            points[i].size = 0.1 + 0.15 * tda_rand();
            points[i].src_port = 0.15 + 0.1 * tda_rand();
            points[i].dst_port = 0.05 + 0.05 * tda_rand();
        }
    } 
    else if (strcmp(profile, "DDoS Attack Vector") == 0) {
        // Form a prominent hollow ring geometry representing targeted flood.
        // This structural ring generates a persistent 1-cycle (H1 void).
        double center_x = 0.5;
        double center_y = 0.55;
        double mean_radius = 0.28;
        
        for (int i = 0; i < n; i++) {
            double angle = ((double)i / n) * 2.0 * M_PI + 0.1 * (tda_rand() - 0.5);
            double width = 0.03 * (tda_rand() - 0.5); // Narrow radial thickness
            double r = mean_radius + width;
            
            points[i].x = center_x + r * cos(angle);
            points[i].y = center_y + r * sin(angle);
            
            // Attack parameters are tightly regular except dest port targeting
            points[i].t = 0.5 + 0.3 * (tda_rand() - 0.5);
            points[i].dt = 0.001; // extremely rapid inter-arrival
            points[i].size = 0.8 + 0.05 * tda_rand(); // massive uniform payload
            points[i].src_port = tda_rand(); // spoofed port addresses
            points[i].dst_port = 0.12; // targeted standard service port (80/443)
        }
    } 
    else if (strcmp(profile, "Stealth Port Scan") == 0) {
        // Linear projections, grid parallel rows representing systematic port sweeps
        int cols = 5;
        int rows = n / cols;
        int idx = 0;
        
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                if (idx >= n) break;
                // Positioned on a discrete grid with minor coordinate offsets
                points[idx].x = 0.15 + 0.16 * c + 0.01 * (tda_rand() - 0.5);
                points[idx].y = 0.20 + 0.13 * r + 0.01 * (tda_rand() - 0.5);
                
                points[idx].t = 0.1 * r + 0.02 * tda_rand();
                points[idx].dt = 0.2; // spaced probes
                points[idx].size = 0.02; // tiny probe size
                points[idx].src_port = 0.85; // single scanning point
                points[idx].dst_port = (double)idx / n; // sweeping across ports
                
                idx++;
            }
        }
        // Fill remaining spaces
        while (idx < n) {
            points[idx].x = 0.5 + 0.3 * (tda_rand() - 0.5);
            points[idx].y = 0.5 + 0.3 * (tda_rand() - 0.5);
            points[idx].t = tda_rand();
            points[idx].dt = 0.2;
            points[idx].size = 0.02;
            points[idx].src_port = 0.85;
            points[idx].dst_port = tda_rand();
            idx++;
        }
    } 
    else if (strcmp(profile, "Slow Exfiltration") == 0) {
        // Two distant clusters connected by an extremely thin, long chain/bridge.
        // Captures gradual slow transfer across otherwise disconnected boundaries.
        int half = n / 2;
        
        // Cluster A
        for (int i = 0; i < half - 3; i++) {
            double radius = 0.06 * tda_rand();
            double angle = tda_rand() * 2.0 * M_PI;
            points[i].x = 0.25 + radius * cos(angle);
            points[i].y = 0.50 + radius * sin(angle);
            
            points[i].t = 0.15 + 0.05 * tda_rand();
            points[i].dt = 0.05 + 0.05 * tda_rand();
            points[i].size = 0.1 + 0.05 * tda_rand();
            points[i].src_port = 0.2;
            points[i].dst_port = 0.05;
        }
        
        // Cluster B
        for (int i = half - 3; i < n - 6; i++) {
            double radius = 0.06 * tda_rand();
            double angle = tda_rand() * 2.0 * M_PI;
            points[i].x = 0.75 + radius * cos(angle);
            points[i].y = 0.50 + radius * sin(angle);
            
            points[i].t = 0.80 + 0.05 * tda_rand();
            points[i].dt = 0.05 + 0.05 * tda_rand();
            points[i].size = 0.1 + 0.05 * tda_rand();
            points[i].src_port = 0.2;
            points[i].dst_port = 0.05;
        }
        
        // Bridges (the slow drip trail)
        int bridge_idx = 0;
        for (int i = n - 6; i < n; i++) {
            double ratio = (double)(bridge_idx + 1) / 7.0;
            // Bridge coordinates stretch horizontally directly from 0.25 to 0.75
            points[i].x = 0.25 + 0.5 * ratio + 0.01 * (tda_rand() - 0.5);
            points[i].y = 0.50 + 0.01 * (tda_rand() - 0.5);
            
            points[i].t = 0.2 + 0.5 * ratio;
            points[i].dt = 0.95; // very large separation/slow timing
            points[i].size = 0.50 + 0.1 * tda_rand(); // massive exfiltrated packets
            points[i].src_port = 0.99; // privileged tunnel port
            points[i].dst_port = 0.44; // remote secure target
            bridge_idx++;
        }
    }
}

double compute_flow_distance(FlowPoint p1, FlowPoint p2) {
    // For TIDS, we calculate Euclidean distance across 2D visual representation 
    // and integrate abstract normalized flow behaviors.
    double dx = p1.x - p2.x;
    double dy = p1.y - p2.y;
    double dt = p1.t - p2.t;
    double ddt = p1.dt - p2.dt;
    double dsize = p1.size - p2.size;
    
    // Weighted combination maximizing visual geometry and behavioral features
    double space_dist = dx * dx + dy * dy;
    double feature_dist = 0.12 * (dt * dt + ddt * ddt + dsize * dsize);
    return sqrt(space_dist + feature_dist);
}

// QSort comparison of Simplices for filtration sorting (ascending birth threshold, ascending dimension)
static int compare_simplices(const void* a, const void* b) {
    const Simplex* sa = (const Simplex*)a;
    const Simplex* sb = (const Simplex*)b;
    
    if (fabs(sa->threshold - sb->threshold) > 1e-9) {
        return (sa->threshold > sb->threshold) ? 1 : -1;
    }
    // If threshold is identical, lower dimension must appear first (face criteria)
    if (sa->dim != sb->dim) {
        return (sa->dim > sb->dim) ? 1 : -1;
    }
    return 0;
}

int construct_vietoris_rips(FlowPoint* points, int num_points, double max_epsilon, Simplex** out_simplices) {
    // Estimate a safe threshold of simplices
    // 0-simplices: num_points
    // 1-simplices: max num_points * (num_points - 1) / 2
    // 2-simplices: max num_points * (num_points - 1) * (num_points - 2) / 6
    // To protect performance in real-time, we allocate a maximum list size
    int max_simplices = num_points + (num_points * (num_points - 1) / 2) + 2000;
    Simplex* simplices = (Simplex*)malloc(max_simplices * sizeof(Simplex));
    if (!simplices) return 0;
    
    int count = 0;
    
    // 1. Add 0-simplices
    for (int i = 0; i < num_points; i++) {
        simplices[count].dim = 0;
        simplices[count].vertices[0] = i;
        simplices[count].threshold = 0.0;
        count++;
    }
    
    // 2. Add 1-simplices (Edges)
    for (int i = 0; i < num_points; i++) {
        for (int j = i + 1; j < num_points; j++) {
            double dist = compute_flow_distance(points[i], points[j]);
            if (dist <= max_epsilon) {
                simplices[count].dim = 1;
                simplices[count].vertices[0] = i;
                simplices[count].vertices[1] = j;
                simplices[count].threshold = dist;
                count++;
                if (count >= max_simplices) break;
            }
        }
        if (count >= max_simplices) break;
    }
    
    // 3. Add 2-simplices (Triangles) if we have safety margins
    if (count < max_simplices - 800) {
        for (int i = 0; i < num_points; i++) {
            for (int j = i + 1; j < num_points; j++) {
                double d_ij = compute_flow_distance(points[i], points[j]);
                if (d_ij > max_epsilon) continue;
                
                for (int k = j + 1; k < num_points; k++) {
                    double d_ik = compute_flow_distance(points[i], points[k]);
                    double d_jk = compute_flow_distance(points[j], points[k]);
                    
                    if (d_ik <= max_epsilon && d_jk <= max_epsilon) {
                        double max_d = d_ij;
                        if (d_ik > max_d) max_d = d_ik;
                        if (d_jk > max_d) max_d = d_jk;
                        
                        simplices[count].dim = 2;
                        simplices[count].vertices[0] = i;
                        simplices[count].vertices[1] = j;
                        simplices[count].vertices[2] = k;
                        simplices[count].threshold = max_d;
                        count++;
                        if (count >= max_simplices) goto allocation_filled;
                    }
                }
            }
        }
    }
    
allocation_filled:
    
    // Sort entire simplex list to guarantee face criteria (all faces listed before parent complexes)
    qsort(simplices, count, sizeof(Simplex), compare_simplices);
    
    *out_simplices = simplices;
    return count;
}

// Recursive helper to check if a sorted array has sub-intervals
static int is_simplex_face(const Simplex* face, const Simplex* parent) {
    if (face->dim != parent->dim - 1) return 0;
    
    int matches = 0;
    int expected = face->dim + 1;
    for (int f = 0; f < expected; f++) {
        for (int p = 0; p < parent->dim + 1; p++) {
            if (face->vertices[f] == parent->vertices[p]) {
                matches++;
                break;
            }
        }
    }
    return (matches == expected);
}

int compute_persistence(Simplex* simplices, int num_simplices, int num_points, PersistencePair** out_pairs) {
    (void)num_points;
    // Array to identify which column has its "lowest one" at a specific row r
    int* low_to_col = (int*)malloc(num_simplices * sizeof(int));
    for (int i = 0; i < num_simplices; i++) low_to_col[i] = -1;
    
    // Allocate columns of boundary matrix in Z_2
    int** cols = (int**)malloc(num_simplices * sizeof(int*));
    int* col_sizes = (int*)calloc(num_simplices, sizeof(int));
    
    // Populate Initial Boundaries
    for (int j = 0; j < num_simplices; j++) {
        int d = simplices[j].dim;
        if (d == 0) {
            cols[j] = NULL;
            col_sizes[j] = 0;
            continue;
        }
        
        // A simplex of dim d has exactly d+1 faces
        cols[j] = (int*)malloc((d + 1) * sizeof(int));
        int count = 0;
        
        // Search previous indices to identify matching codim-1 faces
        for (int i = 0; i < j; i++) {
            if (simplices[i].dim == d - 1) {
                if (is_simplex_face(&simplices[i], &simplices[j])) {
                    cols[j][count++] = i;
                    if (count >= d + 1) break;
                }
            }
        }
        col_sizes[j] = count;
        
        // Sort face indices in descending order to easily track 'low(j)' (which is cols[j][0])
        for (int m = 0; m < count - 1; m++) {
            for (int n = m + 1; n < count; n++) {
                if (cols[j][m] < cols[j][n]) {
                    int tmp = cols[j][m];
                    cols[j][m] = cols[j][n];
                    cols[j][n] = tmp;
                }
            }
        }
    }
    
    // Column Reduction
    for (int j = 0; j < num_simplices; j++) {
        while (col_sizes[j] > 0) {
            int r = cols[j][0]; // low of column j
            int k = low_to_col[r];
            if (k == -1) {
                low_to_col[r] = j; // Pivot found
                break;
            }
            
            // Col k already has low in row r. Vector symmetric sum (XOR merge)
            int* new_col = (int*)malloc((col_sizes[j] + col_sizes[k]) * sizeof(int));
            int size = 0;
            int idx_j = 0, idx_k = 0;
            
            while (idx_j < col_sizes[j] && idx_k < col_sizes[k]) {
                if (cols[j][idx_j] == cols[k][idx_k]) {
                    idx_j++; idx_k++; // XOR cancels equal indices
                } else if (cols[j][idx_j] > cols[k][idx_k]) {
                    new_col[size++] = cols[j][idx_j++];
                } else {
                    new_col[size++] = cols[k][idx_k++];
                }
            }
            while (idx_j < col_sizes[j]) new_col[size++] = cols[j][idx_j++];
            while (idx_k < col_sizes[k]) new_col[size++] = cols[k][idx_k++];
            
            free(cols[j]);
            cols[j] = new_col;
            col_sizes[j] = size;
        }
    }
    
    // Extract Persistence Pairs
    // Make a conservative allocation for pairs:
    // Any paired columns represent a birth-death event.
    PersistencePair* pairs = (PersistencePair*)malloc(num_simplices * sizeof(PersistencePair));
    int pair_count = 0;
    
    // Tracks which simplices have been "killed"
    char* killed = (char*)calloc(num_simplices, sizeof(char));
    
    for (int j = 0; j < num_simplices; j++) {
        if (col_sizes[j] > 0) {
            int birth = cols[j][0];
            int death = j;
            
            pairs[pair_count].dim = simplices[birth].dim;
            pairs[pair_count].birth = simplices[birth].threshold;
            pairs[pair_count].death = simplices[death].threshold;
            pairs[pair_count].birth_simplex = birth;
            pairs[pair_count].death_simplex = death;
            
            killed[birth] = 1;
            pair_count++;
        }
    }
    
    // Any 0-dim or 1-dim simplex that has an empty boundary after reduction 
    // AND was never killed represents a feature with infinite persistence (lives until epsilon max limit)
    for (int i = 0; i < num_simplices; i++) {
        if (simplices[i].dim < 2 && !killed[i]) {
            // Check if boundary was fully cleared
            if (col_sizes[i] == 0) {
                // If it's a 0-simplex, or a 1-simplex not acting as a pivot
                int is_pivot = 0;
                for (int r = 0; r < num_simplices; r++) {
                    if (low_to_col[r] == i) { is_pivot = 1; break; }
                }
                
                if (!is_pivot) {
                    pairs[pair_count].dim = simplices[i].dim;
                    pairs[pair_count].birth = simplices[i].threshold;
                    pairs[pair_count].death = -1.0; // Infinite/Permanent persistence
                    pairs[pair_count].birth_simplex = i;
                    pairs[pair_count].death_simplex = -1;
                    pair_count++;
                }
            }
        }
    }
    
    // Cleanup
    for (int j = 0; j < num_simplices; j++) {
        if (cols[j]) free(cols[j]);
    }
    free(cols);
    free(col_sizes);
    free(low_to_col);
    free(killed);
    
    *out_pairs = pairs;
    return pair_count;
}

void select_landmarks(FlowPoint* points, int num_points, int num_landmarks) {
    if (num_landmarks >= num_points || num_landmarks <= 0) return;
    
    char* selected = (char*)calloc(num_points, sizeof(char));
    int* landmark_indices = (int*)malloc(num_landmarks * sizeof(int));
    
    // Choose first landmark as index 0
    landmark_indices[0] = 0;
    selected[0] = 1;
    
    // Track minimum distance from each point to the selected landmarks
    double* min_dist = (double*)malloc(num_points * sizeof(double));
    for (int i = 0; i < num_points; i++) {
        min_dist[i] = compute_flow_distance(points[i], points[0]);
    }
    
    for (int l = 1; l < num_landmarks; l++) {
        // Find point that maximizes the min_dist
        double max_val = -1.0;
        int best_idx = -1;
        for (int i = 0; i < num_points; i++) {
            if (!selected[i] && min_dist[i] > max_val) {
                max_val = min_dist[i];
                best_idx = i;
            }
        }
        
        // Safety check if all remaining points are overlapping
        if (best_idx == -1) {
            // Find any unselected point
            for (int i = 0; i < num_points; i++) {
                if (!selected[i]) {
                    best_idx = i;
                    break;
                }
            }
        }
        
        if (best_idx >= 0 && best_idx < num_points) {
            landmark_indices[l] = best_idx;
            selected[best_idx] = 1;
            
            // Update min_dist for all points
            for (int i = 0; i < num_points; i++) {
                double d = compute_flow_distance(points[i], points[best_idx]);
                if (d < min_dist[i]) {
                    min_dist[i] = d;
                }
            }
        }
    }
    
    // Permute points so that the selected landmarks are placed at indices 0..num_landmarks-1
    FlowPoint* temp_points = (FlowPoint*)malloc(num_points * sizeof(FlowPoint));
    memcpy(temp_points, points, num_points * sizeof(FlowPoint));
    
    for (int l = 0; l < num_landmarks; l++) {
        points[l] = temp_points[landmark_indices[l]];
    }
    
    int cursor = num_landmarks;
    for (int i = 0; i < num_points; i++) {
        if (!selected[i]) {
            points[cursor++] = temp_points[i];
        }
    }
    
    free(selected);
    free(landmark_indices);
    free(min_dist);
    free(temp_points);
}

int construct_witness_complex(FlowPoint* points, int num_points, int num_landmarks, double max_epsilon, Simplex** out_simplices) {
    if (num_landmarks > num_points) num_landmarks = num_points;
    
    // Max estimate of simplices for L landmarks
    int max_simplices = num_landmarks + (num_landmarks * (num_landmarks - 1) / 2) + 6000;
    Simplex* simplices = (Simplex*)malloc(max_simplices * sizeof(Simplex));
    if (!simplices) return 0;
    
    int count = 0;
    
    // 1. Add 0-simplices (Landmarks)
    for (int i = 0; i < num_landmarks; i++) {
        simplices[count].dim = 0;
        simplices[count].vertices[0] = i;
        simplices[count].threshold = 0.0;
        count++;
    }
    
    // Precompute distance to the nearest landmark for each witness
    double* d0 = (double*)malloc(num_points * sizeof(double));
    for (int w = 0; w < num_points; w++) {
        double min_d = 1e9;
        for (int l = 0; l < num_landmarks; l++) {
            double d = compute_flow_distance(points[w], points[l]);
            if (d < min_d) min_d = d;
        }
        d0[w] = min_d;
    }
    
    // 2. Add 1-simplices (Edges) using the weak witness complex definition:
    // b([l_a, l_b]) = min_{w} max(d(w, l_a), d(w, l_b)) - d0(w)
    double* edge_births = (double*)calloc(num_landmarks * num_landmarks, sizeof(double));
    
    for (int i = 0; i < num_landmarks; i++) {
        for (int j = i + 1; j < num_landmarks; j++) {
            double min_val = 1e9;
            for (int w = 0; w < num_points; w++) {
                double d_ia = compute_flow_distance(points[w], points[i]);
                double d_ib = compute_flow_distance(points[w], points[j]);
                double val = (d_ia > d_ib ? d_ia : d_ib) - d0[w];
                if (val < 0.0) val = 0.0;
                if (val < min_val) {
                    min_val = val;
                }
            }
            
            edge_births[i * num_landmarks + j] = min_val;
            edge_births[j * num_landmarks + i] = min_val;
            
            if (min_val <= max_epsilon) {
                simplices[count].dim = 1;
                simplices[count].vertices[0] = i;
                simplices[count].vertices[1] = j;
                simplices[count].threshold = min_val;
                count++;
                if (count >= max_simplices) break;
            }
        }
        if (count >= max_simplices) break;
    }
    
    // 3. Add 2-simplices (Triangles) using Rips-Witness definition:
    // b([l_a, l_b, l_c]) = max( b([l_a, l_b]), b([l_b, l_c]), b([l_a, l_c]) )
    if (count < max_simplices - 2000) {
        for (int i = 0; i < num_landmarks; i++) {
            for (int j = i + 1; j < num_landmarks; j++) {
                double b_ij = edge_births[i * num_landmarks + j];
                if (b_ij > max_epsilon) continue;
                
                for (int k = j + 1; k < num_landmarks; k++) {
                    double b_ik = edge_births[i * num_landmarks + k];
                    double b_jk = edge_births[j * num_landmarks + k];
                    
                    if (b_ik <= max_epsilon && b_jk <= max_epsilon) {
                        double max_b = b_ij;
                        if (b_ik > max_b) max_b = b_ik;
                        if (b_jk > max_b) max_b = b_jk;
                        
                        simplices[count].dim = 2;
                        simplices[count].vertices[0] = i;
                        simplices[count].vertices[1] = j;
                        simplices[count].vertices[2] = k;
                        simplices[count].threshold = max_b;
                        count++;
                        if (count >= max_simplices) goto allocation_filled;
                    }
                }
            }
        }
    }
    
allocation_filled:
    free(d0);
    free(edge_births);
    
    // Sort simplices to maintain face criteria
    qsort(simplices, count, sizeof(Simplex), compare_simplices);
    
    *out_simplices = simplices;
    return count;
}
