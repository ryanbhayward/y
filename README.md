Y
=
Y player based on the Fuego library using MCTS.

Uses a new connection algorithm:
 - Computes groups, which are sets of pairwise path-connected blocks.
 - Fast enough it is used at all times, even during the playout phase.

Playouts use the SaveBridge pattern. Move selection during the playout
phase is implemented to support weighted-random selection, however, no
weights have been computed at this time, and so all cells are equally
weighted.