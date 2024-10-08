; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt < %s -passes=indvars -S | FileCheck %s

target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: nofree norecurse nounwind uwtable
define void @test(i8* nocapture readnone %arg, i8* noalias nocapture readnone %arg1, i8** noalias nocapture readnone %arg2, i8** noalias nocapture readonly %arg3, i64* noalias nocapture readnone %arg4) local_unnamed_addr #0 {
; CHECK-LABEL: @test
bb:
  %tmp = bitcast i8** %arg3 to [1 x [4 x [10 x [5 x float]]]]**
  %tmp5 = load [1 x [4 x [10 x [5 x float]]]]*, [1 x [4 x [10 x [5 x float]]]]** %tmp, align 8, !invariant.load !0, !dereferenceable !1, !align !2
  %tmp6 = getelementptr inbounds i8*, i8** %arg3, i64 3
  %tmp7 = load i8*, i8** %tmp6, align 8, !invariant.load !0, !dereferenceable !3, !align !2
  %tmp8 = bitcast i8* %tmp7 to [10 x [5 x [2 x [1 x [2 x float]]]]]*
  br label %bb9

bb9:                                              ; preds = %bb33, %bb
  %tmp10 = phi i64 [ 0, %bb ], [ %tmp34, %bb33 ]
  %tmp11 = sub nsw i64 9, %tmp10
  br label %bb12

bb12:                                             ; preds = %bb30, %bb9
  %tmp13 = phi i64 [ 0, %bb9 ], [ %tmp31, %bb30 ]
  %tmp14 = sub nsw i64 4, %tmp13
  br label %bb15

bb15:                                             ; preds = %bb27, %bb12
  %tmp16 = phi i64 [ 0, %bb12 ], [ %tmp28, %bb27 ]
  %tmp17 = mul i64 %tmp16, -2
  %tmp18 = add i64 %tmp17, 2
  br label %bb19

bb19:                                             ; preds = %bb19, %bb15
  %tmp20 = phi i64 [ 0, %bb15 ], [ %tmp25, %bb19 ]
  %tmp21 = add nuw nsw i64 %tmp18, %tmp20
  %tmp22 = getelementptr inbounds [1 x [4 x [10 x [5 x float]]]], [1 x [4 x [10 x [5 x float]]]]* %tmp5, i64 0, i64 0, i64 %tmp21, i64 %tmp11, i64 %tmp14
  %tmp23 = load float, float* %tmp22, align 4, !invariant.load !0, !noalias !4
  %tmp24 = getelementptr inbounds [10 x [5 x [2 x [1 x [2 x float]]]]], [10 x [5 x [2 x [1 x [2 x float]]]]]* %tmp8, i64 0, i64 %tmp10, i64 %tmp13, i64 %tmp16, i64 0, i64 %tmp20
  store float %tmp23, float* %tmp24, align 4, !alias.scope !4, !noalias !7
  %tmp25 = add nuw nsw i64 %tmp20, 1
  %tmp26 = icmp eq i64 %tmp20, 0
  br i1 %tmp26, label %bb19, label %bb27

bb27:                                             ; preds = %bb19
  %tmp28 = add nuw nsw i64 %tmp16, 1
  %tmp29 = icmp eq i64 %tmp16, 0
  br i1 %tmp29, label %bb15, label %bb30

bb30:                                             ; preds = %bb27
  %tmp31 = add nuw nsw i64 %tmp13, 1
  %tmp32 = icmp ugt i64 %tmp13, 3
  br i1 %tmp32, label %bb33, label %bb12

bb33:                                             ; preds = %bb30
  %tmp34 = add nuw nsw i64 %tmp10, 1
  %tmp35 = icmp ugt i64 %tmp10, 8
  br i1 %tmp35, label %bb36, label %bb9

bb36:                                             ; preds = %bb33
  %tmp37 = getelementptr inbounds i8*, i8** %arg3, i64 1
  %tmp38 = bitcast i8** %tmp37 to [1 x [4 x [6 x [7 x float]]]]**
  %tmp39 = load [1 x [4 x [6 x [7 x float]]]]*, [1 x [4 x [6 x [7 x float]]]]** %tmp38, align 8, !invariant.load !0, !dereferenceable !10, !align !2
  %tmp40 = getelementptr inbounds i8, i8* %tmp7, i64 800
  %tmp41 = bitcast i8* %tmp40 to [2 x [6 x [7 x [2 x [1 x float]]]]]*
  br label %bb42

bb42:                                             ; preds = %bb63, %bb36
  %tmp43 = phi i64 [ 0, %bb36 ], [ %tmp64, %bb63 ]
  br label %bb44

bb44:                                             ; preds = %bb60, %bb42
  %tmp45 = phi i64 [ 0, %bb42 ], [ %tmp61, %bb60 ]
  br label %bb46

bb46:                                             ; preds = %bb57, %bb44
  %tmp47 = phi i64 [ 0, %bb44 ], [ %tmp58, %bb57 ]
  br label %bb48

bb48:                                             ; preds = %bb48, %bb46
  %tmp49 = phi i64 [ 0, %bb46 ], [ %tmp55, %bb48 ]
  %tmp50 = shl nuw nsw i64 %tmp49, 1
  %tmp51 = add nuw nsw i64 %tmp50, %tmp43
  %tmp52 = getelementptr inbounds [1 x [4 x [6 x [7 x float]]]], [1 x [4 x [6 x [7 x float]]]]* %tmp39, i64 0, i64 0, i64 %tmp51, i64 %tmp45, i64 %tmp47
  %tmp53 = load float, float* %tmp52, align 4, !invariant.load !0, !noalias !11
  %tmp54 = getelementptr inbounds [2 x [6 x [7 x [2 x [1 x float]]]]], [2 x [6 x [7 x [2 x [1 x float]]]]]* %tmp41, i64 0, i64 %tmp43, i64 %tmp45, i64 %tmp47, i64 %tmp49, i64 0
  store float %tmp53, float* %tmp54, align 4, !alias.scope !11, !noalias !12
  %tmp55 = add nuw nsw i64 %tmp49, 1
  %tmp56 = icmp eq i64 %tmp49, 0
  br i1 %tmp56, label %bb48, label %bb57

bb57:                                             ; preds = %bb48
  %tmp58 = add nuw nsw i64 %tmp47, 1
  %tmp59 = icmp ugt i64 %tmp47, 5
  br i1 %tmp59, label %bb60, label %bb46

bb60:                                             ; preds = %bb57
  %tmp61 = add nuw nsw i64 %tmp45, 1
  %tmp62 = icmp ugt i64 %tmp45, 4
  br i1 %tmp62, label %bb63, label %bb44

bb63:                                             ; preds = %bb60
  %tmp64 = add nuw nsw i64 %tmp43, 1
  %tmp65 = icmp eq i64 %tmp43, 0
  br i1 %tmp65, label %bb42, label %bb66

bb66:                                             ; preds = %bb63
  %tmp67 = getelementptr inbounds i8, i8* %tmp7, i64 1472
  %tmp68 = bitcast i8* %tmp67 to [2 x [1 x [2 x [2 x [2 x float]]]]]*
  br label %bb69

bb69:                                             ; preds = %bb140, %bb66
  %tmp70 = phi i64 [ 0, %bb66 ], [ %tmp141, %bb140 ]
  br label %bb71

bb71:                                             ; preds = %bb137, %bb69
  %tmp72 = phi i64 [ 0, %bb69 ], [ %tmp138, %bb137 ]
  %tmp73 = shl nuw nsw i64 %tmp72, 1
  %tmp74 = add nsw i64 %tmp73, -2
  br label %bb75

bb75:                                             ; preds = %bb134, %bb71
  %tmp76 = phi i64 [ 0, %bb71 ], [ %tmp135, %bb134 ]
  %tmp77 = add nsw i64 %tmp76, -1
  br label %bb78

bb78:                                             ; preds = %bb129, %bb75
  %tmp79 = phi i64 [ 0, %bb75 ], [ %tmp132, %bb129 ]
  br label %bb80

bb80:                                             ; preds = %bb125, %bb78
  %tmp81 = phi float [ 0.000000e+00, %bb78 ], [ %tmp126, %bb125 ]
  %tmp82 = phi i64 [ 0, %bb78 ], [ %tmp127, %bb125 ]
  %tmp83 = shl nuw nsw i64 %tmp82, 1
  %tmp84 = add nsw i64 %tmp83, -1
  %tmp85 = icmp ult i64 %tmp84, 10
  %tmp86 = sub nsw i64 5, %tmp82
  br i1 %tmp85, label %bb88, label %bb87

bb87:                                             ; preds = %bb80
  br label %bb124

bb88:                                             ; preds = %bb80
  br label %bb89

bb89:                                             ; preds = %bb100, %bb88
  %tmp90 = phi float [ %tmp101, %bb100 ], [ %tmp81, %bb88 ]
  %tmp91 = phi i64 [ %tmp102, %bb100 ], [ 0, %bb88 ]
  %tmp92 = add i64 %tmp74, %tmp91
  %tmp93 = icmp ult i64 %tmp92, 5
  %tmp94 = sub nsw i64 6, %tmp91
  br i1 %tmp93, label %bb96, label %bb95

bb95:                                             ; preds = %bb89
  br label %bb99

bb96:                                             ; preds = %bb89
  br label %bb104

bb97:                                             ; preds = %bb110
  %tmp98 = phi float [ %tmp111, %bb110 ]
  br label %bb100

bb99:                                             ; preds = %bb95
  br label %bb100

bb100:                                            ; preds = %bb99, %bb97
  %tmp101 = phi float [ %tmp98, %bb97 ], [ %tmp90, %bb99 ]
  %tmp102 = add nuw nsw i64 %tmp91, 1
  %tmp103 = icmp ugt i64 %tmp91, 5
  br i1 %tmp103, label %bb122, label %bb89

bb104:                                            ; preds = %bb110, %bb96
  %tmp105 = phi float [ %tmp111, %bb110 ], [ %tmp90, %bb96 ]
  %tmp106 = phi i64 [ %tmp112, %bb110 ], [ 0, %bb96 ]
  %tmp107 = shl nuw nsw i64 %tmp106, 1
  ; CHECK-NOT: %bugged = add nuw nsw
  ; CHECK:     %bugged = add nsw
  %bugged = add i64 %tmp77, %tmp107
  %tmp109 = icmp ult i64 %bugged, 2
  br i1 %tmp109, label %bb114, label %bb110

bb110:                                            ; preds = %bb114, %bb104
  %tmp111 = phi float [ %tmp121, %bb114 ], [ %tmp105, %bb104 ]
  %tmp112 = add nuw nsw i64 %tmp106, 1
  %tmp113 = icmp eq i64 %tmp106, 0
  br i1 %tmp113, label %bb104, label %bb97

bb114:                                            ; preds = %bb104
  %tmp115 = sub nsw i64 1, %tmp106
  %tmp116 = getelementptr inbounds [2 x [6 x [7 x [2 x [1 x float]]]]], [2 x [6 x [7 x [2 x [1 x float]]]]]* %tmp41, i64 0, i64 %tmp70, i64 %tmp86, i64 %tmp94, i64 %tmp115, i64 0
  %tmp117 = getelementptr inbounds [10 x [5 x [2 x [1 x [2 x float]]]]], [10 x [5 x [2 x [1 x [2 x float]]]]]* %tmp8, i64 0, i64 %tmp84, i64 %tmp92, i64 %bugged, i64 0, i64 %tmp79
  %tmp118 = load float, float* %tmp117, align 4, !alias.scope !4, !noalias !7
  %tmp119 = load float, float* %tmp116, align 4, !alias.scope !11, !noalias !12
  %tmp120 = fmul reassoc nsz contract float %tmp118, %tmp119
  %tmp121 = fadd reassoc nsz contract float %tmp105, %tmp120
  br label %bb110

bb122:                                            ; preds = %bb100
  %tmp123 = phi float [ %tmp101, %bb100 ]
  br label %bb125

bb124:                                            ; preds = %bb87
  br label %bb125

bb125:                                            ; preds = %bb124, %bb122
  %tmp126 = phi float [ %tmp123, %bb122 ], [ %tmp81, %bb124 ]
  %tmp127 = add nuw nsw i64 %tmp82, 1
  %tmp128 = icmp ugt i64 %tmp82, 4
  br i1 %tmp128, label %bb129, label %bb80

bb129:                                            ; preds = %bb125
  %tmp130 = phi float [ %tmp126, %bb125 ]
  %tmp131 = getelementptr inbounds [2 x [1 x [2 x [2 x [2 x float]]]]], [2 x [1 x [2 x [2 x [2 x float]]]]]* %tmp68, i64 0, i64 %tmp70, i64 0, i64 %tmp72, i64 %tmp76, i64 %tmp79
  store float %tmp130, float* %tmp131, align 4, !alias.scope !13, !noalias !14
  %tmp132 = add nuw nsw i64 %tmp79, 1
  %tmp133 = icmp eq i64 %tmp79, 0
  br i1 %tmp133, label %bb78, label %bb134

bb134:                                            ; preds = %bb129
  %tmp135 = add nuw nsw i64 %tmp76, 1
  %tmp136 = icmp eq i64 %tmp76, 0
  br i1 %tmp136, label %bb75, label %bb137

bb137:                                            ; preds = %bb134
  %tmp138 = add nuw nsw i64 %tmp72, 1
  %tmp139 = icmp eq i64 %tmp72, 0
  br i1 %tmp139, label %bb71, label %bb140

bb140:                                            ; preds = %bb137
  %tmp141 = add nuw nsw i64 %tmp70, 1
  %tmp142 = icmp eq i64 %tmp70, 0
  br i1 %tmp142, label %bb69, label %bb143

bb143:                                            ; preds = %bb140
  %tmp144 = getelementptr inbounds i8*, i8** %arg3, i64 2
  %tmp145 = bitcast i8** %tmp144 to [4 x [2 x [1 x [2 x float]]]]**
  %tmp146 = load [4 x [2 x [1 x [2 x float]]]]*, [4 x [2 x [1 x [2 x float]]]]** %tmp145, align 8, !invariant.load !0, !dereferenceable !16, !align !2
  br label %bb147

bb147:                                            ; preds = %bb143
  br label %bb148

bb148:                                            ; preds = %bb147
  br label %bb149

bb149:                                            ; preds = %bb148
  %tmp150 = getelementptr inbounds [2 x [1 x [2 x [2 x [2 x float]]]]], [2 x [1 x [2 x [2 x [2 x float]]]]]* %tmp68, i64 0, i64 0, i64 0, i64 0, i64 0, i64 0
  %tmp151 = load float, float* %tmp150, align 4, !alias.scope !13, !noalias !14
  %tmp152 = getelementptr inbounds [4 x [2 x [1 x [2 x float]]]], [4 x [2 x [1 x [2 x float]]]]* %tmp146, i64 0, i64 0, i64 0, i64 0, i64 0
  store float %tmp151, float* %tmp152, align 4, !alias.scope !17, !noalias !13
  %tmp153 = getelementptr inbounds [2 x [1 x [2 x [2 x [2 x float]]]]], [2 x [1 x [2 x [2 x [2 x float]]]]]* %tmp68, i64 0, i64 0, i64 0, i64 1, i64 0, i64 0
  %tmp154 = load float, float* %tmp153, align 4, !alias.scope !13, !noalias !14
  %tmp155 = getelementptr inbounds [4 x [2 x [1 x [2 x float]]]], [4 x [2 x [1 x [2 x float]]]]* %tmp146, i64 0, i64 0, i64 0, i64 0, i64 1
  store float %tmp154, float* %tmp155, align 4, !alias.scope !17, !noalias !13
  br label %bb156

bb156:                                            ; preds = %bb149
  %tmp157 = getelementptr inbounds [2 x [1 x [2 x [2 x [2 x float]]]]], [2 x [1 x [2 x [2 x [2 x float]]]]]* %tmp68, i64 0, i64 1, i64 0, i64 0, i64 0, i64 0
  %tmp158 = load float, float* %tmp157, align 4, !alias.scope !13, !noalias !14
  %tmp159 = getelementptr inbounds [4 x [2 x [1 x [2 x float]]]], [4 x [2 x [1 x [2 x float]]]]* %tmp146, i64 0, i64 0, i64 1, i64 0, i64 0
  store float %tmp158, float* %tmp159, align 4, !alias.scope !17, !noalias !13
  %tmp160 = getelementptr inbounds [2 x [1 x [2 x [2 x [2 x float]]]]], [2 x [1 x [2 x [2 x [2 x float]]]]]* %tmp68, i64 0, i64 1, i64 0, i64 1, i64 0, i64 0
  %tmp161 = load float, float* %tmp160, align 4, !alias.scope !13, !noalias !14
  %tmp162 = getelementptr inbounds [4 x [2 x [1 x [2 x float]]]], [4 x [2 x [1 x [2 x float]]]]* %tmp146, i64 0, i64 0, i64 1, i64 0, i64 1
  store float %tmp161, float* %tmp162, align 4, !alias.scope !17, !noalias !13
  br label %bb163

bb163:                                            ; preds = %bb156
  br label %bb164

bb164:                                            ; preds = %bb163
  %tmp165 = getelementptr inbounds [2 x [1 x [2 x [2 x [2 x float]]]]], [2 x [1 x [2 x [2 x [2 x float]]]]]* %tmp68, i64 0, i64 0, i64 0, i64 0, i64 0, i64 1
  %tmp166 = load float, float* %tmp165, align 4, !alias.scope !13, !noalias !14
  %tmp167 = getelementptr inbounds [4 x [2 x [1 x [2 x float]]]], [4 x [2 x [1 x [2 x float]]]]* %tmp146, i64 0, i64 1, i64 0, i64 0, i64 0
  store float %tmp166, float* %tmp167, align 4, !alias.scope !17, !noalias !13
  %tmp168 = getelementptr inbounds [2 x [1 x [2 x [2 x [2 x float]]]]], [2 x [1 x [2 x [2 x [2 x float]]]]]* %tmp68, i64 0, i64 0, i64 0, i64 1, i64 0, i64 1
  %tmp169 = load float, float* %tmp168, align 4, !alias.scope !13, !noalias !14
  %tmp170 = getelementptr inbounds [4 x [2 x [1 x [2 x float]]]], [4 x [2 x [1 x [2 x float]]]]* %tmp146, i64 0, i64 1, i64 0, i64 0, i64 1
  store float %tmp169, float* %tmp170, align 4, !alias.scope !17, !noalias !13
  br label %bb171

bb171:                                            ; preds = %bb164
  %tmp172 = getelementptr inbounds [2 x [1 x [2 x [2 x [2 x float]]]]], [2 x [1 x [2 x [2 x [2 x float]]]]]* %tmp68, i64 0, i64 1, i64 0, i64 0, i64 0, i64 1
  %tmp173 = load float, float* %tmp172, align 4, !alias.scope !13, !noalias !14
  %tmp174 = getelementptr inbounds [4 x [2 x [1 x [2 x float]]]], [4 x [2 x [1 x [2 x float]]]]* %tmp146, i64 0, i64 1, i64 1, i64 0, i64 0
  store float %tmp173, float* %tmp174, align 4, !alias.scope !17, !noalias !13
  %tmp175 = getelementptr inbounds [2 x [1 x [2 x [2 x [2 x float]]]]], [2 x [1 x [2 x [2 x [2 x float]]]]]* %tmp68, i64 0, i64 1, i64 0, i64 1, i64 0, i64 1
  %tmp176 = load float, float* %tmp175, align 4, !alias.scope !13, !noalias !14
  %tmp177 = getelementptr inbounds [4 x [2 x [1 x [2 x float]]]], [4 x [2 x [1 x [2 x float]]]]* %tmp146, i64 0, i64 1, i64 1, i64 0, i64 1
  store float %tmp176, float* %tmp177, align 4, !alias.scope !17, !noalias !13
  br label %bb178

bb178:                                            ; preds = %bb171
  br label %bb179

bb179:                                            ; preds = %bb178
  %tmp180 = getelementptr inbounds [2 x [1 x [2 x [2 x [2 x float]]]]], [2 x [1 x [2 x [2 x [2 x float]]]]]* %tmp68, i64 0, i64 0, i64 0, i64 0, i64 1, i64 0
  %tmp181 = load float, float* %tmp180, align 4, !alias.scope !13, !noalias !14
  %tmp182 = getelementptr inbounds [4 x [2 x [1 x [2 x float]]]], [4 x [2 x [1 x [2 x float]]]]* %tmp146, i64 0, i64 2, i64 0, i64 0, i64 0
  store float %tmp181, float* %tmp182, align 4, !alias.scope !17, !noalias !13
  %tmp183 = getelementptr inbounds [2 x [1 x [2 x [2 x [2 x float]]]]], [2 x [1 x [2 x [2 x [2 x float]]]]]* %tmp68, i64 0, i64 0, i64 0, i64 1, i64 1, i64 0
  %tmp184 = load float, float* %tmp183, align 4, !alias.scope !13, !noalias !14
  %tmp185 = getelementptr inbounds [4 x [2 x [1 x [2 x float]]]], [4 x [2 x [1 x [2 x float]]]]* %tmp146, i64 0, i64 2, i64 0, i64 0, i64 1
  store float %tmp184, float* %tmp185, align 4, !alias.scope !17, !noalias !13
  br label %bb186

bb186:                                            ; preds = %bb179
  %tmp187 = getelementptr inbounds [2 x [1 x [2 x [2 x [2 x float]]]]], [2 x [1 x [2 x [2 x [2 x float]]]]]* %tmp68, i64 0, i64 1, i64 0, i64 0, i64 1, i64 0
  %tmp188 = load float, float* %tmp187, align 4, !alias.scope !13, !noalias !14
  %tmp189 = getelementptr inbounds [4 x [2 x [1 x [2 x float]]]], [4 x [2 x [1 x [2 x float]]]]* %tmp146, i64 0, i64 2, i64 1, i64 0, i64 0
  store float %tmp188, float* %tmp189, align 4, !alias.scope !17, !noalias !13
  %tmp190 = getelementptr inbounds [2 x [1 x [2 x [2 x [2 x float]]]]], [2 x [1 x [2 x [2 x [2 x float]]]]]* %tmp68, i64 0, i64 1, i64 0, i64 1, i64 1, i64 0
  %tmp191 = load float, float* %tmp190, align 4, !alias.scope !13, !noalias !14
  %tmp192 = getelementptr inbounds [4 x [2 x [1 x [2 x float]]]], [4 x [2 x [1 x [2 x float]]]]* %tmp146, i64 0, i64 2, i64 1, i64 0, i64 1
  store float %tmp191, float* %tmp192, align 4, !alias.scope !17, !noalias !13
  br label %bb193

bb193:                                            ; preds = %bb186
  br label %bb194

bb194:                                            ; preds = %bb193
  %tmp195 = getelementptr inbounds [2 x [1 x [2 x [2 x [2 x float]]]]], [2 x [1 x [2 x [2 x [2 x float]]]]]* %tmp68, i64 0, i64 0, i64 0, i64 0, i64 1, i64 1
  %tmp196 = load float, float* %tmp195, align 4, !alias.scope !13, !noalias !14
  %tmp197 = getelementptr inbounds [4 x [2 x [1 x [2 x float]]]], [4 x [2 x [1 x [2 x float]]]]* %tmp146, i64 0, i64 3, i64 0, i64 0, i64 0
  store float %tmp196, float* %tmp197, align 4, !alias.scope !17, !noalias !13
  %tmp198 = getelementptr inbounds [2 x [1 x [2 x [2 x [2 x float]]]]], [2 x [1 x [2 x [2 x [2 x float]]]]]* %tmp68, i64 0, i64 0, i64 0, i64 1, i64 1, i64 1
  %tmp199 = load float, float* %tmp198, align 4, !alias.scope !13, !noalias !14
  %tmp200 = getelementptr inbounds [4 x [2 x [1 x [2 x float]]]], [4 x [2 x [1 x [2 x float]]]]* %tmp146, i64 0, i64 3, i64 0, i64 0, i64 1
  store float %tmp199, float* %tmp200, align 4, !alias.scope !17, !noalias !13
  br label %bb201

bb201:                                            ; preds = %bb194
  %tmp202 = getelementptr inbounds [2 x [1 x [2 x [2 x [2 x float]]]]], [2 x [1 x [2 x [2 x [2 x float]]]]]* %tmp68, i64 0, i64 1, i64 0, i64 0, i64 1, i64 1
  %tmp203 = load float, float* %tmp202, align 4, !alias.scope !13, !noalias !14
  %tmp204 = getelementptr inbounds [4 x [2 x [1 x [2 x float]]]], [4 x [2 x [1 x [2 x float]]]]* %tmp146, i64 0, i64 3, i64 1, i64 0, i64 0
  store float %tmp203, float* %tmp204, align 4, !alias.scope !17, !noalias !13
  %tmp205 = getelementptr inbounds [2 x [1 x [2 x [2 x [2 x float]]]]], [2 x [1 x [2 x [2 x [2 x float]]]]]* %tmp68, i64 0, i64 1, i64 0, i64 1, i64 1, i64 1
  %tmp206 = load float, float* %tmp205, align 4, !alias.scope !13, !noalias !14
  %tmp207 = getelementptr inbounds [4 x [2 x [1 x [2 x float]]]], [4 x [2 x [1 x [2 x float]]]]* %tmp146, i64 0, i64 3, i64 1, i64 0, i64 1
  store float %tmp206, float* %tmp207, align 4, !alias.scope !17, !noalias !13
  ret void
}

attributes #0 = { nofree norecurse nounwind uwtable "denormal-fp-math"="preserve-sign" "no-frame-pointer-elim"="false" }

!0 = !{}
!1 = !{i64 800}
!2 = !{i64 16}
!3 = !{i64 1536}
!4 = !{!5}
!5 = !{!"buffer: {index:3, offset:0, size:800}", !6}
!6 = !{!"XLA global AA domain"}
!7 = !{!8, !9}
!8 = !{!"buffer: {index:3, offset:800, size:672}", !6}
!9 = !{!"buffer: {index:3, offset:1472, size:64}", !6}
!10 = !{i64 672}
!11 = !{!8}
!12 = !{!5, !9}
!13 = !{!9}
!14 = !{!15, !5, !8}
!15 = !{!"buffer: {index:2, offset:0, size:64}", !6}
!16 = !{i64 64}
!17 = !{!15}
