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
#? [a4 b3 east b7 west]

28 group_blocks b12
#? [b12 west south]

29 group_blocks h9
#? [j10 h9 g7 east]


#
# Check that blocks with semis to two different members of a group are
# added to the group
#
loadsgf sgf/vc/check-triangle.sgf
30 group_blocks d7
#? [.*c8.*]

#
# Check that friendly groups are recomputed properly next to a move
#
loadsgf sgf/vc/check-friendly-group-break-simple.sgf
31 group_blocks e9
#? [e9]

32 group_blocks f8
#? [f8 d7]

#
# Check that opponent groups are recomputed properly
#
loadsgf sgf/vc/check-opp-group-break-simple.sgf
33 group_blocks e9
#? [d8 d10]

34 group_blocks e7
#? [e7 d5 east]

loadsgf sgf/vc/check-opp-group-break-into-three.sgf
35 group_blocks g9
#? [g9 east]

36 group_blocks d9
#? [d9]

37 group_blocks f10
#? [f10]*

