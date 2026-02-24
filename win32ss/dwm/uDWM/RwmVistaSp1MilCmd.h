#pragma once

/*
 * Vista SP1 MILCMD numeric IDs.
 *
 * IMPORTANT:
 * - Our workspace includes a newer MILCore protocol header set (later .NET/WPF).
 * - When uDWM is tested against Vista SP1's milcore.dll, the *numeric command IDs*
 *   must match Vista, even if the structs are layout-compatible.
 *
 * Evidence (from DwmReversing\\Vista\\dwmredir.dll.c, DuceHelper::*):
 * - Visual_SetTransform:     cmd.Type = 35
 * - Visual_SetClip:          cmd.Type = 36
 * - Visual_SetRenderOptions: cmd.Type = 38
 * - Visual_SetContent:       cmd.Type = 39
 * - Visual_RemoveChild:      cmd.Type = 42
 * - Visual_InsertChildAt:    cmd.Type = 43
 *
 * Evidence (from DwmReversing\\Vista\\milcore.dll.c resource factory switch):
 * - CreateResource(TYPE_COLORRESOURCE) uses resource type id 54
 */

#define RWM_MILCMD_VSP1_VISUAL_SETTRANSFORM      (35u)
#define RWM_MILCMD_VSP1_VISUAL_SETCLIP           (36u)
/* Visual_SetAlpha not directly observed in dwmredir; inferred ordering. */
#define RWM_MILCMD_VSP1_VISUAL_SETALPHA          (37u)
#define RWM_MILCMD_VSP1_VISUAL_SETRENDEROPTIONS  (38u)
#define RWM_MILCMD_VSP1_VISUAL_SETCONTENT        (39u)
/* Not directly observed yet; inferred from command ordering in WGX headers. */
#define RWM_MILCMD_VSP1_VISUAL_REMOVEALLCHILDREN (41u)
#define RWM_MILCMD_VSP1_VISUAL_REMOVECHILD       (42u)
#define RWM_MILCMD_VSP1_VISUAL_INSERTCHILDAT     (43u)

/* Visual creation (milcore command dispatch: case 0x21). */
#define RWM_MILCMD_VSP1_VISUAL_CREATE            (33u)

/* Transport sync flush (Vista uDWM sends cmd=3, sizeof(cmd)=4 before commit). */
#define RWM_MILCMD_VSP1_TRANSPORT_SYNCFLUSH      (3u)

/* Resource type id (CreateResource), NOT a command id. */
#define RWM_MILRT_VSP1_COLORRESOURCE             (54u)

/*
 * Transform creation/update (Vista SP1):
 * - DuceHelper::CreateTranslateTransform uses cmd.Type = 163 and CreateResource
 *   type id 68 for TYPE_TRANSLATETRANSFORM.
 */
#define RWM_MILCMD_VSP1_TRANSLATETRANSFORM       (163u)

/*
 * Vista SP1 resource type IDs (as used by DuceHelper::CreateResource).
 * Only define what uDWM actively creates for interop.
 */
#define RWM_MILRT_VSP1_TRANSLATETRANSFORM        (68u)
/* milcore resource factory: case 39 constructs CMilVisual (not currently used). */
#define RWM_MILRT_VSP1_VISUAL                    (39u)

/*
 * Vista SP1 uDWM resource type IDs (from DwmReversing\\Vista\\uDWM.dll.c):
 * - TYPE_RENDERDATA:        44
 * - TYPE_SOLIDCOLORBRUSH:   81
 *
 * These differ from newer WPF headers.
 */
#define RWM_MILRT_VSP1_RENDERDATA                (44u)
#define RWM_MILRT_VSP1_SOLIDCOLORBRUSH           (81u)

/*
 * Geometry resources used by Vista uDWM for glass/blur:
 * - TYPE_RECTANGLEGEOMETRY: 75
 * - Update rectangle geometry: cmd.Type = 169, sizeof=0x48
 */
#define RWM_MILRT_VSP1_RECTANGLEGEOMETRY         (75u)
#define RWM_MILCMD_VSP1_RECTANGLEGEOMETRY        (169u)

/*
 * ColorResource update (Vista uDWM / DesktopManager::SetColorizationColorResource):
 * - cmd.Type = 19, sizeof=0x18, payload: 4 floats (scRGB)
 */
#define RWM_MILCMD_VSP1_COLORRESOURCE            (19u)

/*
 * Vista SP1 command IDs used by uDWM render-data path (from DwmReversing\\Vista\\uDWM.dll.c):
 * - RenderData update:          cmd.Type = 29, sizeof=0x0c, extra = cmd.cbData
 * - SolidColorBrush update:     cmd.Type = 174, sizeof=0x30
 * - RenderData draw-rectangle:  instruction type = 111 (inside RenderData stream)
 */
#define RWM_MILCMD_VSP1_RENDERDATA               (29u)
#define RWM_MILCMD_VSP1_SOLIDCOLORBRUSH          (174u)
#define RWM_MILDRAW_VSP1_RECTANGLE               (111u)

/*
 * Desktop render target creation / root binding (Vista SP1 dwmredir):
 * - CreateResource(TYPE_DESKTOPRENDERTARGET) uses resource type id 48
 * - CreateDesktopTarget sends cmd.Type = 73, size 0x5c
 * - SetRoot sends cmd.Type = 77, size 0x0c
 */
#define RWM_MILRT_VSP1_DESKTOPRENDERTARGET       (48u)
#define RWM_MILCMD_VSP1_DESKTOPTARGET_CREATE     (73u)
#define RWM_MILCMD_VSP1_TARGET_SETROOT           (77u)
/*
 * Target commands closely follow SetRoot (77) in Vista SP1.
 * We need Invalidate to actually schedule presents.
 */
#define RWM_MILCMD_VSP1_TARGET_SETCLEARCOLOR     (78u)
#define RWM_MILCMD_VSP1_TARGET_INVALIDATE        (79u)
#define RWM_MILCMD_VSP1_TARGET_SETFLAGS          (80u)

/*
 * Window render target (Vista SP1 dwmredir DuceHelper::CreateWindowTarget):
 * - CreateResource(TYPE_WINDOWRENDERTARGET) uses resource type id 50
 * - Uses the same cmd.Type = 73, size 0x5c as desktop target create, but fills
 *   HWND and dimensions.
 */
#define RWM_MILRT_VSP1_WINDOWRENDERTARGET        (50u)

/* Composition node used by Vista DWM redirection. */
#define RWM_MILRT_VSP1_WINDOWNODE                (42u)

/*
 * WindowNode commands (Vista SP1 dwmredir DuceHelper):
 * - WindowNode_SetBounds:        cmd.Type = 58, size 0x38
 * - WindowNode_SetSpriteImage:   cmd.Type = 62, size 0x0C
 * - WindowNode_UpdateSpriteHandle cmd.Type = 60, size 0x10
 * - WindowNode_SetSpriteClip:    cmd.Type = 64, size 0x10
 * - WindowNode_SetAlphaMargins:  cmd.Type = 67, size 0x18
 */
#define RWM_MILCMD_VSP1_WINDOWNODE_SETBOUNDS     (58u)
#define RWM_MILCMD_VSP1_WINDOWNODE_UPDATESPRITE  (60u)
#define RWM_MILCMD_VSP1_WINDOWNODE_SETSPRITEIMAGE (62u)
/*
 * WindowNode_UpdateProperties (Vista SP1 dwmredir DuceHelper::WindowNode_UpdateProperties):
 * - After SetSpriteImage(62), Vista sends two 0x0C commands:
 *   - cmd.Type = 63: sets the DX surface handle for the WindowNode (may be 0)
 *   - cmd.Type = 65: sets the DX clip handle for the WindowNode (may be 0)
 */
#define RWM_MILCMD_VSP1_WINDOWNODE_SETDXSURFACE   (63u)
#define RWM_MILCMD_VSP1_WINDOWNODE_SETSPRITECLIP (64u)
#define RWM_MILCMD_VSP1_WINDOWNODE_SETDXCLIP      (65u)
/* WindowNode source modifications (Vista RefreshNodeProperties): cmd.Type = 66, size 0x2C. */
#define RWM_MILCMD_VSP1_WINDOWNODE_SETSOURCEMODS  (66u)
#define RWM_MILCMD_VSP1_WINDOWNODE_SETALPHAMARGINS (67u)

/*
 * Window redirection surface margin updates (Vista SP1 dwmredir DuceHelper):
 * - WindowRedirection_GdiSpriteBitmap_UpdateMargins: cmd.Type = 91, size 0x18
 * - WindowRedirection_FlipChain_UpdateMargins:       cmd.Type = 97, size 0x18
 */
#define RWM_MILCMD_VSP1_WNREDIR_GDISPRITE_UPDATEMARGINS (91u)
#define RWM_MILCMD_VSP1_WNREDIR_FLIPCHAIN_UPDATEMARGINS (97u)

/*
 * WindowNode create (Vista SP1 dwmredir DuceHelper::WindowNode_Create):
 * - cmd.Type = 54, size 0x1C (7 dwords)
 *   NOTE: This numeric ID conflicts with older assumptions about ColorResource.
 *   We trust dwmredir here because it shows the exact send size and layout.
 */
#define RWM_MILCMD_VSP1_WINDOWNODE_CREATE        (54u)

/*
 * Geometry resources used for clipping (Vista SP1 dwmredir DuceHelper):
 * - CreateResource(TYPE_PATHGEOMETRY) uses resource type id 79
 * - UpdateGeometryFromRegionData uses cmd.Type = 173 (MilChannel_BeginCommand sizeof=0x14)
 */
#define RWM_MILRT_VSP1_PATHGEOMETRY              (79u)
#define RWM_MILCMD_VSP1_PATHGEOMETRY             (173u)

