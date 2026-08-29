# SMBv3 Independent Building Object Image Match Baseline

- Available ImageGen atlases: 18 / 18
- Objects with references: 108 / 108
- Object isolation hard gate: passed
- Measured comparisons: 600 / 648
- Isolated observability failures: 48
- Objects passing every required view: 0 / 108
- Required per-view score: 80.00%

| Visual Object Model | Objects | Measured Views | Passed Objects | Mean View Score | Weakest Independent Tests |
|---|---:|---:|---:|---:|---|
| `stair` | 2 | 11 | 0 | 27.79% | silhouette_overlap 1.26%, edge_alignment 4.56%, component_count 14.79% |
| `safety_equipment` | 3 | 18 | 0 | 32.21% | silhouette_overlap 5.99%, edge_alignment 11.49%, component_count 31.57% |
| `chair` | 2 | 10 | 0 | 33.61% | edge_alignment 8.69%, silhouette_overlap 11.29%, symmetry 31.68% |
| `privacy_screen` | 1 | 6 | 0 | 34.87% | edge_alignment 0.80%, silhouette_overlap 1.52%, aspect_ratio 25.57% |
| `solar_equipment` | 2 | 9 | 0 | 36.86% | silhouette_overlap 0.40%, edge_alignment 4.12%, aspect_ratio 10.62% |
| `television` | 1 | 6 | 0 | 37.24% | edge_alignment 11.72%, silhouette_overlap 15.24%, component_count 24.80% |
| `cabinet` | 3 | 14 | 0 | 38.00% | silhouette_overlap 9.74%, edge_alignment 13.03%, symmetry 44.46% |
| `desk` | 1 | 6 | 0 | 38.53% | edge_alignment 11.67%, silhouette_overlap 17.51%, component_count 21.27% |
| `wardrobe` | 2 | 12 | 0 | 39.62% | edge_alignment 10.65%, silhouette_overlap 21.36%, component_count 37.05% |
| `exterior_wall` | 5 | 30 | 0 | 39.97% | silhouette_overlap 11.76%, edge_alignment 12.84%, aspect_ratio 23.63% |
| `sink` | 1 | 6 | 0 | 40.09% | edge_alignment 8.62%, silhouette_overlap 23.42%, component_count 35.17% |
| `window` | 4 | 22 | 0 | 40.14% | silhouette_overlap 12.46%, edge_alignment 18.64%, aspect_ratio 35.22% |
| `door` | 1 | 6 | 0 | 42.80% | edge_alignment 13.92%, aspect_ratio 31.36%, silhouette_overlap 37.87% |
| `shower` | 2 | 9 | 0 | 42.88% | edge_alignment 9.44%, component_count 29.05%, silhouette_overlap 50.14% |
| `appliance` | 2 | 12 | 0 | 42.90% | edge_alignment 11.93%, component_count 34.58%, silhouette_overlap 35.60% |
| `service_equipment` | 9 | 53 | 0 | 43.31% | silhouette_overlap 20.88%, edge_alignment 23.21%, component_count 40.49% |
| `kitchen` | 1 | 6 | 0 | 43.81% | silhouette_overlap 1.79%, edge_alignment 6.98%, aspect_ratio 32.86% |
| `light` | 6 | 33 | 0 | 43.88% | silhouette_overlap 12.81%, edge_alignment 15.00%, component_count 39.54% |
| `structural_assembly` | 8 | 32 | 0 | 43.94% | silhouette_overlap 19.37%, edge_alignment 20.06%, component_count 38.90% |
| `ground_surface` | 1 | 5 | 0 | 44.89% | edge_alignment 3.85%, silhouette_overlap 30.21%, aspect_ratio 44.34% |
| `structural_floor` | 6 | 36 | 0 | 46.83% | silhouette_overlap 23.57%, edge_alignment 29.14%, aspect_ratio 38.52% |
| `vegetation` | 5 | 30 | 0 | 50.05% | edge_alignment 6.84%, silhouette_overlap 30.08%, aspect_ratio 43.66% |
| `table` | 1 | 6 | 0 | 52.14% | edge_alignment 16.11%, component_count 26.93%, silhouette_overlap 47.41% |
| `system_aggregate` | 6 | 35 | 0 | 53.04% | silhouette_overlap 12.29%, edge_alignment 19.57%, component_count 53.96% |
| `room_assembly` | 9 | 51 | 0 | 53.55% | edge_alignment 15.21%, silhouette_overlap 28.49%, component_count 52.23% |
| `fence` | 2 | 12 | 0 | 54.60% | edge_alignment 22.72%, silhouette_overlap 27.22%, aspect_ratio 46.48% |
| `study` | 1 | 6 | 0 | 55.28% | edge_alignment 16.81%, silhouette_overlap 34.02%, symmetry 51.22% |
| `building_assembly` | 5 | 30 | 0 | 55.95% | edge_alignment 23.25%, silhouette_overlap 32.22%, aspect_ratio 53.48% |
| `bed` | 2 | 12 | 0 | 57.00% | edge_alignment 16.74%, silhouette_overlap 50.25%, aspect_ratio 61.07% |
| `paving` | 3 | 18 | 0 | 57.02% | silhouette_overlap 35.02%, edge_alignment 37.33%, component_count 45.41% |
| `vanity` | 2 | 11 | 0 | 57.50% | edge_alignment 15.28%, silhouette_overlap 50.55%, aspect_ratio 67.94% |
| `downpipe` | 1 | 6 | 0 | 59.70% | silhouette_overlap 8.68%, edge_alignment 22.56%, symmetry 55.21% |
| `interior_wall` | 3 | 14 | 0 | 59.91% | edge_alignment 34.49%, silhouette_overlap 40.29%, component_count 47.26% |
| `toilet` | 2 | 9 | 0 | 61.37% | edge_alignment 26.15%, silhouette_overlap 60.97%, aspect_ratio 64.16% |
| `site_assembly` | 1 | 6 | 0 | 63.04% | edge_alignment 24.10%, silhouette_overlap 33.46%, component_count 67.26% |
| `canopy` | 1 | 6 | 0 | 63.62% | edge_alignment 39.31%, silhouette_overlap 55.06%, aspect_ratio 61.22% |
| `couch` | 1 | 6 | 0 | 66.84% | edge_alignment 22.72%, component_count 64.35%, silhouette_overlap 69.04% |

This is a diagnostic baseline, not certification. Missing references, failed independent tests, and scores below 80% remain explicit.
