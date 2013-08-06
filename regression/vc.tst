#-----------------------------------------------------------------------------
# VC tests
#-----------------------------------------------------------------------------

#
# Check basic bridges in middle of board
#
loadsgf sgf/vc/check-bridge.sgf
10 carrier_between h10 g11
#? [g10 h11]

11 carrier_between c9 d8
#? [c8 d9]

12 carrier_between d8 c6
#? [c7 e7]

13 group_blocks a1
#? [west east a1]

14 group_blocks d8
#? [c6 d8 c9]

15 group_blocks h10
#? [h10 g11]

#
# Check basic bridges with edge of board
#
loadsgf sgf/vc/check-bridge-with-edge.sgf
20 carrier_between b3 west
#? [a2 a3]

21 carrier_between b3 east
#? [b2 c3]

22 carrier_between a4 west
#? []

23 carrier_between b7 west
#? [a5 a7]

24 carrier_between b12 west
#? [a11 a12]

25 carrier_between b12 south
#? [b13 c13]

26 carrier_between h9 east
#? [h8 i9]

27 group_blocks b3
#? [west east b3 a4 b7]

28 group_blocks b12
#? [west south b12]

29 group_blocks h9
#? [east g7 h9 j10]

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
#? [west east south e7 b10]

#
# Check that opponent groups are recomputed properly
#
loadsgf sgf/vc/check-opp-group-break-simple.sgf
60 group_blocks e9
#? [d8 d10]

61 group_blocks e7
#? [east d5 e7]

loadsgf sgf/vc/check-opp-group-break-into-three.sgf
62 group_blocks g9
#? [east g9]

63 group_blocks d9
#? [d9]

64 group_blocks f10
#? [f10]
