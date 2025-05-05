[BetterVR_UpdateProjectionMatrix_V208]
moduleMatches = 0x6267BFD0

.origin = codecave


updateCameraPositionAndTarget:
; repeat instructions from either branches
lfs f0, 0xEC0(r31)
stfs f0, 0x5CC(r31)
lfs f13, 0xEB8(r31)
stfs f13, 0x5C4(r31)

; function prologue
mflr r0
stwu r1, -0x58(r1)
stw r0, 0x5C(r1)

bl import.coreinit.hook_updateCameraOLD

exit_updateCameraPositionAndTarget:
; function epilogue
lwz r0, 0x5C(r1)
mtlr r0
addi r1, r1, 0x58

addi r3, r31, 0xE78
blr


0x02C054FC = bla updateCameraPositionAndTarget
0x02C05590 = bla updateCameraPositionAndTarget


loadLineFormat:
.string "sead::PerspectiveProjection::setFovY(this = 0x%08x fovRadians = %f offsetX = %f near = %f far = %f) LR = 0x%08x %c"
printSetFovY:
mflr r0
stwu r1, -0x20(r1)
stw r0, 0x24(r1)

stw r3, 0x8(r1)
stw r4, 0xC(r1)
stw r5, 0x10(r1)
stfs f1, 0x14(r1)
stfs f2, 0x18(r1)
stfs f3, 0x1C(r1)
stfs f4, 0x20(r1)

lfs f2, 0xB0(r3) ; load offsetX
lfs f3, 0x94(r3) ; load near
lfs f4, 0x98(r3) ; load far

mr r4, r3
lwz r5, 0x10+0x24(r1) ; load LR from parent stack
lis r3, loadLineFormat@ha
addi r3, r3, loadLineFormat@l
bl printToLog

fmuls f31, f1, f0

lwz r3, 0x8(r1)
lwz r4, 0xC(r1)
lwz r5, 0x10(r1)

lfs f1, 0x14(r1)
lfs f2, 0x18(r1)
lfs f3, 0x1C(r1)
lfs f4, 0x20(r1)

lwz r0, 0x24(r1)
mtlr r0
addi r1, r1, 0x20
blr

;0x030C16F8 = bla printSetFovY