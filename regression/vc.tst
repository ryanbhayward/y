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

200 full_connected_with c6 black
#? [b4 c4 a5 b5 c5 d5 e5 b6 e6 b7 c7 e7 f7 d8]

201 full_connected_with e10 black
#? [d8 c9]

202 semi_connected_with e11 white
#? [c11 g11]

203 semi_connected_with e11 black
#? [c9]

204 semi_connected_with d7 white
#? [b6 e6 b7 f7 c8 f8]


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

127 full_connected_with west black
#? [a1 a2 b2 a3 b3 a4 a5 a7 b7 a8 b8 a9 b9 a10 b10 a11 b11 a12 a13 b13]

128 full_connected_with west white
#? [a1 a2 b2 a3 a5 a6 a7 a8 b8 a9 b9 a10 b10 a11 b11 a12 b12 a13 b13]

129 full_connected_with a3 black
#? [west b3 a4]

130 full_connected_with b4 black
#? [b3 a4]

131 semi_connected_with west black
#? []

132 semi_connected_with west white
#? [b4 b5]

133 semi_connected_with b5 white
#? [west a6]

134 carrier_between f12 south
#? [f13 g13]

135 carrier_between g12 south
#? [g13]

136 full_connected_with g12 black
#? []

137 full_connected_with g12 white
#? [h13]

138 semi_connected_with g12 black
#? [south i13]

139 semi_connected_with g12 white
#? []

140 carrier_between h12 south
#? []

141 full_connected_with h12 black
#? [i13]

142 full_connected_with h12 white
#? [h13]

143 semi_connected_with h12 black
#? []

144 semi_connected_with h12 white
#? []

27 group_blocks b3
#? [west east b3 a4 b7]

28 group_blocks b12
#? [west south b12]

29 group_blocks h9
#? [east g7 h9 j10]*

loadsgf sgf/vc/check-edge-shared-liberties.sgf
300 carrier_between f7 east
#? []

301 full_connected_with f7 white
#? [.*east.*]

302 carrier_between b7 west
#? []

303 full_connected_with b7 black
#? [.*west.*]

#
# Check that when merging A into B, all liberties of A now have
# empty full connections to B.
# 
loadsgf sgf/vc/check-connections-after-merge.sgf
400 carrier_between d7 g9
#? []

401 full_connected_with d7 black
#? [g10]

402 carrier_between e9 g9
#? []

403 full_connected_with e9 black
#? [g10]


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
