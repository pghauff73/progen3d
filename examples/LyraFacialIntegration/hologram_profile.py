"""Declared hologram settings for the external Blender inspection adapter.

The geometry grammar and its renderer metadata are separate contracts. This
module consumes the actual declaration and records the adapter's explicit
artist-unit conversions. It does not imply native ProGen3D viewport parity.
"""
from __future__ import annotations

import hashlib
import json
import math
from pathlib import Path
import re


class HologramPresentationProfile:
    """A pinned declaration and its deterministic presentation-time settings."""

    REQUIRED_PARAMETERS = (
        'brightness', 'grain_scale', 'grain_speed', 'grain_strength',
        'rim_strength', 'scan_spacing', 'scan_speed', 'scan_strength',
        'pulse_speed', 'bloom_threshold', 'bloom_mix', 'projector_strength',
    )
    NONNEGATIVE_PARAMETERS = (
        'brightness', 'grain_strength', 'rim_strength', 'scan_strength',
        'pulse_speed', 'bloom_threshold', 'projector_strength',
    )
    ADAPTER_CONSTANTS = {
        'surfaceOpacity': .98,
        'baseEmission': .025,
        'pulseBaseline': .9,
        'pulseAmplitude': .1,
        'grainArtistUnitMultiplier': .01,
        'projectorArtistUnitMultiplier': .1,
    }

    def __init__(self, grammar_path, grammar_sha256, declaration_text,
                 declaration_line, declared, presentation_time, seed):
        self._grammar_path = str(grammar_path)
        self._grammar_sha256 = grammar_sha256
        self._declaration_line = declaration_line
        self._declaration_sha256 = hashlib.sha256(declaration_text.encode()).hexdigest()
        self._declared = declared
        canonical = json.dumps(declared, sort_keys=True, separators=(',', ':'), allow_nan=False)
        self._profile_sha256 = hashlib.sha256(canonical.encode()).hexdigest()
        self._time = presentation_time
        self._seed = seed
        constants = self.ADAPTER_CONSTANTS
        pulse_phase = 2 * math.pi * declared['pulse_speed'] * presentation_time
        if not math.isfinite(pulse_phase):
            raise ValueError('The declared pulse phase exceeds the finite presentation domain.')
        self._effective = {
            'surfaceOpacity': constants['surfaceOpacity'],
            'baseEmission': constants['baseEmission'],
            'brightness': declared['brightness'],
            'pulseFactor': constants['pulseBaseline'] + constants['pulseAmplitude'] * math.cos(pulse_phase),
            'rimStrength': declared['rim_strength'],
            'scanSpacingM': declared['scan_spacing'],
            'scanPhaseRadians': -2 * math.pi * declared['scan_speed'] * presentation_time / declared['scan_spacing'],
            'scanStrength': declared['scan_strength'],
            'grainScalePerM': declared['grain_scale'],
            'grainStrength': constants['grainArtistUnitMultiplier'] * declared['grain_strength'],
            'noiseW': seed + declared['grain_speed'] * presentation_time,
            'bloomThreshold': declared['bloom_threshold'],
            'bloomMix': declared['bloom_mix'],
            'projectorEnergy': constants['projectorArtistUnitMultiplier'] * declared['projector_strength'],
        }
        if not all(math.isfinite(value) for value in self._effective.values()):
            raise ValueError('The declared settings overflow the finite presentation domain.')

    @staticmethod
    def _reject_constant(value):
        raise ValueError('Nonfinite JSON constant in hologram profile: ' + value)

    @staticmethod
    def _unique_object(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError('Duplicate hologram parameter: ' + key)
            result[key] = value
        return result

    @classmethod
    def read(cls, grammar_path, presentation_time, seed=0):
        grammar_path = Path(grammar_path).resolve()
        grammar_bytes = grammar_path.read_bytes()
        text = grammar_bytes.decode('utf-8')
        declarations = [(index + 1, match.group(1).strip())
                        for index, line in enumerate(text.splitlines())
                        if (match := re.match(r'^\s*#\s*@hologram\b(.*)$', line))]
        if len(declarations) != 1:
            raise ValueError('A hologram render requires exactly one # @hologram JSON declaration.')
        line, declaration_text = declarations[0]
        try:
            declared = json.loads(declaration_text, parse_constant=cls._reject_constant,
                                  object_pairs_hook=cls._unique_object)
        except json.JSONDecodeError as error:
            raise ValueError(f'Invalid hologram JSON at grammar line {line}: {error.msg}') from error
        if not isinstance(declared, dict):
            raise ValueError('The hologram declaration must be a JSON object.')
        missing = sorted(set(cls.REQUIRED_PARAMETERS) - set(declared))
        if missing:
            raise ValueError('The declared profile is missing required parameters: ' + ', '.join(missing))
        for name in cls.REQUIRED_PARAMETERS:
            value = declared[name]
            if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value):
                raise ValueError('Hologram parameter ' + name + ' must be a finite number.')
        for name in cls.NONNEGATIVE_PARAMETERS:
            if declared[name] < 0:
                raise ValueError('Hologram parameter ' + name + ' must be nonnegative.')
        if declared['grain_scale'] <= 0 or declared['scan_spacing'] <= 0:
            raise ValueError('Hologram grain_scale and scan_spacing must be positive.')
        if not -1 <= declared['bloom_mix'] <= 1:
            raise ValueError('Hologram bloom_mix must be in [-1,1] for the Blender glare adapter.')
        if (isinstance(presentation_time, bool) or not isinstance(presentation_time, (int, float))
                or not math.isfinite(presentation_time)):
            raise ValueError('Presentation time must be a finite number of seconds.')
        if isinstance(seed, bool) or not isinstance(seed, int) or not 0 <= seed <= 2147483647:
            raise ValueError('The presentation seed must be an integer in [0,2147483647].')
        return cls(grammar_path, hashlib.sha256(grammar_bytes).hexdigest(), declaration_text,
                   line, declared, float(presentation_time), seed)

    @property
    def effective(self):
        # Callers cannot mutate the receipt by changing a material-settings copy.
        return dict(self._effective)

    def receipt(self):
        unsupported = sorted(set(self._declared) - set(self.REQUIRED_PARAMETERS))
        return {
            'schema': 'progen3d.lyra-hologram-presentation-profile.v1',
            'grammar': self._grammar_path,
            'grammarSha256': self._grammar_sha256,
            'profileSha256': self._profile_sha256,
            'declarationTextSha256': self._declaration_sha256,
            'declarationLine': self._declaration_line,
            'declared': json.loads(json.dumps(self._declared, allow_nan=False)),
            'presentationTimeSeconds': self._time,
            'seed': self._seed,
            'effective': dict(self._effective),
            'adapterConstants': dict(self.ADAPTER_CONSTANTS),
            'declaredDefaultsApplied': {},
            'unsupportedKeys': unsupported,
            'unsupportedKeyPolicy': 'Recorded and unused; never silently interpreted as a supported shader control.',
            'conversions': {
                'brightness': 'Multiplies the complete emission sum together with pulseFactor.',
                'pulseFactor': '.9 + .1*cos(2*pi*pulse_speed*presentation_time); amplitude is an adapter constant.',
                'scanPhaseRadians': '-2*pi*scan_speed*presentation_time/scan_spacing; adapter interprets scan_speed as metres per second along world +Y.',
                'scanSpacingM': 'scan_spacing; stripe angular frequency is 2*pi/scan_spacing.',
                'grainStrength': '.01*grain_strength; explicit conversion from authored artist units to inspection emission strength.',
                'noiseW': 'seed + grain_speed*presentation_time; fourth coordinate of the 4D noise field.',
                'projectorEnergy': '.1*projector_strength; explicit artist-unit conversion for the hologram-only cyan upward directional projector.',
                'directParameters': 'rim_strength, scan_strength, grain_scale, bloom_threshold and bloom_mix are consumed without scaling.',
            },
            'scope': 'External Blender inspection adapter. This does not establish native ProGen3D viewport shader parity. Geometry time and presentation time are recorded separately.',
        }
