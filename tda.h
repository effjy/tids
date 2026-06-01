#ifndef TDA_H
#define TDA_H

/**
 * Representation of a 5D Network Flow embedding point.
 */
typedef struct {
    double x; // Visualization Coordinate X (normalized)
    double y; // Visualization Coordinate Y (normalized)
    
    // Abstract Flow Coordinates:
    double t;       // time
    double dt;      // inter-arrival time
    double size;    // volume of bytes
    double src_port;// source port
    double dst_port;// destination port
} FlowPoint;

/**
 * Representation of a Simplex (vertex, edge, triangle) in the Vietoris-Rips filtration.
 */
typedef struct {
    int vertices[3];  // Simplex vertices (up to dimension 2, i.e., triangles)
    int dim;          // Dimension (0 = point, 1 = edge, 2 = triangle)
    double threshold; // Filtration value (epsilon) at which this simplex is born
} Simplex;

/**
 * Representation of parsed Birth-Death coordinates.
 */
typedef struct {
    int dim;          // Homology dimension (0 or 1)
    double birth;     // Filtration birth threshold
    double death;     // Filtration death threshold (or -1 mapping to infinite/persistent)
    int birth_simplex;// Simplex index that created the feature
    int death_simplex;// Simplex index that closed the feature
} PersistencePair;

/**
 * Generate simulated points corresponding to topological profiles of network behaviors.
 */
void generate_flow_points(FlowPoint* points, int n, const char* profile);

/**
 * Calculate the Euclidean distance of abstract flow positions.
 */
double compute_flow_distance(FlowPoint p1, FlowPoint p2);

/**
 * Perform Max-Min Landmark Selection to choose 'num_landmarks' points
 * and reorder the 'points' array so that they reside at indices 0..num_landmarks-1.
 */
void select_landmarks(FlowPoint* points, int num_points, int num_landmarks);

/**
 * Construct the Vietoris-Rips complexes under the threshold constraint.
 */
int construct_vietoris_rips(FlowPoint* points, int num_points, double max_epsilon, Simplex** out_simplices);

/**
 * Construct a Landmark-based Witness Complex under the threshold constraint,
 * with the first 'num_landmarks' points treated as landmarks and all 'num_points' as witnesses.
 */
int construct_witness_complex(FlowPoint* points, int num_points, int num_landmarks, double max_epsilon, Simplex** out_simplices);

/**
 * Perform boundary reduction in Z_2 using column actions.
 */
int compute_persistence(Simplex* simplices, int num_simplices, int num_points, PersistencePair** out_pairs);

#endif // TDA_H
