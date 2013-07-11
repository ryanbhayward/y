#-----------------------------------------------------------------------------
# VC tests
#-----------------------------------------------------------------------------

#
# Check basic bridges in middle of board
#
loadsgf sgf/vc/check-bridge.sgf
10 block_shared_liberties h10 g11
#? [g10 h11]

11 block_shared_liberties c9 d8
#? [c8 d9]

12 block_shared_liberties d8 c6
#? [c7 e7]

13 group_blocks a1
#? [a1 west east]

14 group_blocks d8
#? [c6 d8 c9]

15 group_blocks h10
#? [h10 g11]

#
# Check basic bridges with edge of board
#
loadsgf sgf/vc/check-bridge-with-edge.sgf
20 block_shared_liberties b3 west
#? [a2 a3]

21 block_shared_liberties b3 east
#? [b2 c3]

22 block_shared_liberties a4 west
#? []

23 block_shared_liberties b7 west
#? [a5 a7]

24 block_shared_liberties b12 west
#? [a11 a12]

25 block_shared_liberties b12 south
#? [b13 c13]

26 block_shared_liberties h9 east
#? [h8 i9]

27 group_blocks b3
#? [b3 a4 b7 west east]

28 group_blocks b12
#? [b12 west south]

29 group_blocks h9
#? [g7 h9 j10 east]

#
# Check that blocks with semis to two different members of a group are
# added to the group
#
loadsgf sgf/vc/check-triangle.sgf
30 group_blocks d7
#? [.*c8.*]

#
# Semis to the edge should not prevent connecting to a block
# touching the edge.
#
loadsgf sgf/vc/semi-with-edge-conflict.sgf 19
40 group_blocks e6
#? [.*d4.*]

#
# Check that friendly groups are recomputed properly next to a move
#
loadsgf sgf/vc/check-friendly-group-break-simple.sgf
50 group_blocks e9
#? [e9]

51 group_blocks f8
#? [d7 f8]

# Last move merges two blocks together: then friendly group search
# is using out-of-date block anchors temporarily. This check ensures
# the group search is correct in such a case.
loadsgf sgf/vc/check-friendly-group-break-blockmerge.sgf 18
55 group_blocks c8
#? [e7 b10 west east south]

#
# Check that opponent groups are recomputed properly
#
loadsgf sgf/vc/check-opp-group-break-simple.sgf
60 group_blocks e9
#? [d8 d10]

61 group_blocks e7
#? [d5 e7 east]

loadsgf sgf/vc/check-opp-group-break-into-three.sgf
62 group_blocks g9
#? [g9 east]

63 group_blocks d9
#? [d9]

64 group_blocks f10
#? [f10]
