/******************************************************************************/
/**
 * @file        AudioSliderWrappers.cs
 * @project     Pulse Protocol
 * @author      Leu Jun Yong (primary) - 100%
 *
 * @brief       Small wrapper scripts for different slider types using the shared slider core.
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
/******************************************************************************/
using ScriptAPI;

// ==========================
// KNOBS
// ==========================
public class VolumeKnob : AudioSliderKnobBase { protected override string FixedTag => "Volume"; }
public class MusicKnob  : AudioSliderKnobBase { protected override string FixedTag => "Music"; }
public class SfxKnob    : AudioSliderKnobBase { protected override string FixedTag => "SFX"; }
public class BrightnessKnob : AudioSliderKnobBase { protected override string FixedTag => "Brightness"; }
public class LatencyKnob : AudioSliderKnobBase { protected override string FixedTag => "Latency"; }

// ==========================
// FILLS
// ==========================
public class VolumeFill : AudioSliderFillBase { protected override string FixedTag => "Volume"; }
public class MusicFill  : AudioSliderFillBase { protected override string FixedTag => "Music"; }
public class SfxFill    : AudioSliderFillBase { protected override string FixedTag => "SFX"; }
public class BrightnessFill : AudioSliderFillBase { protected override string FixedTag => "Brightness"; }
public class LatencyFill : AudioSliderFillBase { protected override string FixedTag => "Latency"; }
