/**
 * Turbo Tuning — Audio Engine
 * Web Audio API Synthesizer for Harmonica, Brass, Strings, and Woodwinds
 * 
 * Features:
 *   - Multi-instrument synthesis
 *   - 4-band EQ (Low, Mid, High, Presence)
 *   - Effects: Reverb, Delay, Chorus
 *   - Safe Mode (compressor/limiter)
 *   - Auto-Tune pitch correction
 *   - Voice management
 */

class AudioEngine {
    // Standard Richter-tuned diatonic harmonica in C
    // MIDI note numbers for holes 1-10
    static HARMONICA_BLOW = [60, 64, 67, 72, 76, 79, 84, 88, 91, 96];
    static HARMONICA_DRAW = [62, 67, 71, 74, 77, 81, 83, 86, 89, 93];

    // Circle of Fifths — semitone offsets from C
    static KEYS = ['C', 'G', 'D', 'A', 'E', 'B', 'F#', 'Db', 'Ab', 'Eb', 'Bb', 'F'];
    static KEY_OFFSETS = { C:0, G:7, D:2, A:9, E:4, B:11, 'F#':6, Db:1, Ab:8, Eb:3, Bb:10, F:5 };
    static RELATIVE_MINORS = {
        C:'Am', G:'Em', D:'Bm', A:'F#m', E:'C#m', B:'G#m',
        'F#':'D#m', Db:'Bbm', Ab:'Fm', Eb:'Cm', Bb:'Gm', F:'Dm'
    };

    // Note names for display
    static NOTE_NAMES = ['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'];

    constructor() {
        this.ctx = null;
        this.masterGain = null;
        this.compressor = null;
        this.eqNodes = {};
        this.reverbConvolver = null;
        this.reverbGain = null;
        this.reverbDry = null;
        this.delayNode = null;
        this.delayFeedback = null;
        this.delayGain = null;
        this.chorusNodes = null;
        this.chorusGain = null;
        this.activeVoices = new Map();
        this.currentKey = 'C';
        this.currentInstrument = 'harmonica';
        this.safeModeEnabled = false;
        this.autoTuneEnabled = true;
        this.isInitialized = false;
    }

    /**
     * Initialize the audio context and create the processing chain.
     */
    async init() {
        if (this.isInitialized) return;

        this.ctx = new (window.AudioContext || window.webkitAudioContext)();

        // === Master Gain ===
        this.masterGain = this.ctx.createGain();
        this.masterGain.gain.value = 0.7;
        this.masterGain.connect(this.ctx.destination);

        // === Compressor (Safe Mode) ===
        this.compressor = this.ctx.createDynamicsCompressor();
        this.compressor.threshold.value = -6;
        this.compressor.knee.value = 30;
        this.compressor.ratio.value = 3;
        this.compressor.attack.value = 0.003;
        this.compressor.release.value = 0.25;
        this.compressor.connect(this.masterGain);

        // === EQ Chain: Low → Mid → High → Presence ===
        this.eqNodes.low = this.ctx.createBiquadFilter();
        this.eqNodes.low.type = 'lowshelf';
        this.eqNodes.low.frequency.value = 200;
        this.eqNodes.low.gain.value = 0;

        this.eqNodes.mid = this.ctx.createBiquadFilter();
        this.eqNodes.mid.type = 'peaking';
        this.eqNodes.mid.frequency.value = 1000;
        this.eqNodes.mid.Q.value = 1;
        this.eqNodes.mid.gain.value = 0;

        this.eqNodes.high = this.ctx.createBiquadFilter();
        this.eqNodes.high.type = 'highshelf';
        this.eqNodes.high.frequency.value = 4000;
        this.eqNodes.high.gain.value = 0;

        this.eqNodes.presence = this.ctx.createBiquadFilter();
        this.eqNodes.presence.type = 'peaking';
        this.eqNodes.presence.frequency.value = 3200;
        this.eqNodes.presence.Q.value = 0.8;
        this.eqNodes.presence.gain.value = 0;

        // Chain EQ filters
        this.eqNodes.low.connect(this.eqNodes.mid);
        this.eqNodes.mid.connect(this.eqNodes.high);
        this.eqNodes.high.connect(this.eqNodes.presence);

        // === Reverb (Convolver) ===
        this.reverbConvolver = this.ctx.createConvolver();
        this.reverbConvolver.buffer = this._createReverbIR(2.5, 2.2);

        this.reverbGain = this.ctx.createGain();
        this.reverbGain.gain.value = 0.25;

        this.reverbDry = this.ctx.createGain();
        this.reverbDry.gain.value = 1.0;

        // === Delay ===
        this.delayNode = this.ctx.createDelay(2.0);
        this.delayNode.delayTime.value = 0.35;

        this.delayFeedback = this.ctx.createGain();
        this.delayFeedback.gain.value = 0.35;

        this.delayGain = this.ctx.createGain();
        this.delayGain.gain.value = 0.0; // Off by default

        // === Chorus (simple detune effect) ===
        this.chorusDelay1 = this.ctx.createDelay(0.05);
        this.chorusDelay1.delayTime.value = 0.015;
        this.chorusDelay2 = this.ctx.createDelay(0.05);
        this.chorusDelay2.delayTime.value = 0.025;

        this.chorusGain = this.ctx.createGain();
        this.chorusGain.gain.value = 0.0; // Off by default

        // LFO for chorus modulation
        this.chorusLFO = this.ctx.createOscillator();
        this.chorusLFO.type = 'sine';
        this.chorusLFO.frequency.value = 1.5;
        this.chorusLFOGain = this.ctx.createGain();
        this.chorusLFOGain.gain.value = 0.002;
        this.chorusLFO.connect(this.chorusLFOGain);
        this.chorusLFOGain.connect(this.chorusDelay1.delayTime);
        this.chorusLFOGain.connect(this.chorusDelay2.delayTime);
        this.chorusLFO.start();

        // === Connect the routing ===
        // EQ output → dry path
        this.eqNodes.presence.connect(this.reverbDry);

        // EQ output → reverb wet path
        this.eqNodes.presence.connect(this.reverbConvolver);
        this.reverbConvolver.connect(this.reverbGain);

        // EQ output → delay wet path
        this.eqNodes.presence.connect(this.delayNode);
        this.delayNode.connect(this.delayFeedback);
        this.delayFeedback.connect(this.delayNode);
        this.delayNode.connect(this.delayGain);

        // EQ output → chorus wet path
        this.eqNodes.presence.connect(this.chorusDelay1);
        this.eqNodes.presence.connect(this.chorusDelay2);
        this.chorusDelay1.connect(this.chorusGain);
        this.chorusDelay2.connect(this.chorusGain);

        // All paths → compressor
        this.reverbDry.connect(this.compressor);
        this.reverbGain.connect(this.compressor);
        this.delayGain.connect(this.compressor);
        this.chorusGain.connect(this.compressor);

        this.isInitialized = true;
        console.log('[AudioEngine] Initialized — sample rate:', this.ctx.sampleRate);
    }

    /**
     * Generate an impulse response buffer for convolution reverb.
     */
    _createReverbIR(duration, decayRate) {
        const sr = this.ctx.sampleRate;
        const length = Math.floor(sr * duration);
        const buffer = this.ctx.createBuffer(2, length, sr);

        for (let ch = 0; ch < 2; ch++) {
            const data = buffer.getChannelData(ch);
            for (let i = 0; i < length; i++) {
                data[i] = (Math.random() * 2 - 1) * Math.pow(1 - i / length, decayRate);
            }
        }
        return buffer;
    }

    /**
     * Convert MIDI note number to frequency.
     */
    midiToFreq(midi) {
        return 440 * Math.pow(2, (midi - 69) / 12);
    }

    /**
     * Get the note name and octave from MIDI note number.
     */
    midiToNoteName(midi) {
        const note = AudioEngine.NOTE_NAMES[midi % 12];
        const octave = Math.floor(midi / 12) - 1;
        return { note, octave };
    }

    /**
     * Get the MIDI note for a given hole, action, and current key.
     */
    getNote(hole, action) {
        const baseNotes = (action === 'blow')
            ? AudioEngine.HARMONICA_BLOW
            : AudioEngine.HARMONICA_DRAW;
        const baseMidi = baseNotes[hole - 1];
        const offset = AudioEngine.KEY_OFFSETS[this.currentKey] || 0;
        return baseMidi + offset;
    }

    /**
     * Get all notes for the current key (for display purposes).
     */
    getAllNotes(action) {
        const baseNotes = (action === 'blow')
            ? AudioEngine.HARMONICA_BLOW
            : AudioEngine.HARMONICA_DRAW;
        const offset = AudioEngine.KEY_OFFSETS[this.currentKey] || 0;
        return baseNotes.map(n => n + offset);
    }

    // ==========================================================
    //  PLAY / STOP
    // ==========================================================

    /**
     * Play a note for a harmonica hole.
     * @param {number} hole - Hole number (1-10)
     * @param {string} action - 'blow' or 'draw'
     * @param {number} velocity - 0.0 to 1.0
     * @returns {{ midi: number, note: string, octave: number }}
     */
    playNote(hole, action, velocity = 0.8) {
        if (!this.isInitialized) return null;

        const voiceId = `${hole}-${action}`;

        // Stop existing voice on this hole/action
        if (this.activeVoices.has(voiceId)) {
            this._releaseVoice(voiceId);
        }

        const midiNote = this.getNote(hole, action);
        let freq = this.midiToFreq(midiNote);

        // Apply auto-tune: snap to nearest chromatic pitch (already quantized by MIDI)
        // In a wind-controller scenario, this would snap continuous pitch input
        if (this.autoTuneEnabled) {
            const nearestMidi = Math.round(midiNote);
            freq = this.midiToFreq(nearestMidi);
        }

        // Create voice based on current instrument
        let voice;
        switch (this.currentInstrument) {
            case 'trumpet':
            case 'tuba':
            case 'french_horn':
                voice = this._createBrassVoice(freq, velocity, this.currentInstrument);
                break;
            case 'guitar':
            case 'harp':
            case 'violin':
                voice = this._createStringVoice(freq, velocity, this.currentInstrument);
                break;
            case 'flute':
            case 'clarinet':
            case 'pan_flute':
                voice = this._createWoodwindVoice(freq, velocity, this.currentInstrument);
                break;
            case 'harmonica':
            default:
                voice = this._createHarmonicaVoice(freq, velocity, action);
                break;
        }

        // Connect voice output to the EQ input
        voice.output.connect(this.eqNodes.low);
        this.activeVoices.set(voiceId, voice);

        const { note, octave } = this.midiToNoteName(midiNote);
        return { midi: midiNote, note, octave };
    }

    /**
     * Stop a note for a harmonica hole.
     */
    stopNote(hole, action) {
        this._releaseVoice(`${hole}-${action}`);
    }

    /**
     * Stop all active voices.
     */
    stopAll() {
        for (const [id] of this.activeVoices) {
            this._releaseVoice(id);
        }
    }

    /**
     * Release a voice with a short fade-out.
     */
    _releaseVoice(voiceId) {
        const voice = this.activeVoices.get(voiceId);
        if (!voice) return;

        const now = this.ctx.currentTime;
        const releaseTime = 0.08;

        try {
            voice.envelope.gain.cancelScheduledValues(now);
            voice.envelope.gain.setValueAtTime(voice.envelope.gain.value, now);
            voice.envelope.gain.linearRampToValueAtTime(0, now + releaseTime);
        } catch (e) { /* ignore */ }

        // Schedule cleanup
        setTimeout(() => {
            voice.oscillators.forEach(osc => { try { osc.stop(); } catch(e) {} });
            voice.sources.forEach(src => { try { src.stop(); } catch(e) {} });
            try { voice.output.disconnect(); } catch(e) {}
        }, (releaseTime + 0.05) * 1000);

        this.activeVoices.delete(voiceId);
    }

    // ==========================================================
    //  VOICE CREATION — Harmonica
    // ==========================================================

    _createHarmonicaVoice(freq, velocity, action) {
        const now = this.ctx.currentTime;

        // Sawtooth + detuned square for reed character
        const osc1 = this.ctx.createOscillator();
        osc1.type = 'sawtooth';
        osc1.frequency.value = freq;

        const osc2 = this.ctx.createOscillator();
        osc2.type = 'square';
        osc2.frequency.value = freq * 1.003; // slight detune for shimmer

        // Sine sub for body
        const osc3 = this.ctx.createOscillator();
        osc3.type = 'sine';
        osc3.frequency.value = freq;

        // Breath noise
        const noiseBuffer = this.ctx.createBuffer(1, this.ctx.sampleRate * 0.5, this.ctx.sampleRate);
        const noiseData = noiseBuffer.getChannelData(0);
        for (let i = 0; i < noiseData.length; i++) {
            noiseData[i] = Math.random() * 2 - 1;
        }
        const noise = this.ctx.createBufferSource();
        noise.buffer = noiseBuffer;
        noise.loop = true;

        // Individual gains
        const g1 = this.ctx.createGain(); g1.gain.value = 0.28 * velocity;
        const g2 = this.ctx.createGain(); g2.gain.value = 0.12 * velocity;
        const g3 = this.ctx.createGain(); g3.gain.value = 0.22 * velocity;
        const gNoise = this.ctx.createGain(); gNoise.gain.value = 0.04 * velocity;

        // Tone filter
        const filter = this.ctx.createBiquadFilter();
        filter.type = 'lowpass';
        filter.frequency.value = action === 'blow' ? 2800 : 3800;
        filter.Q.value = 2.5;

        // Envelope
        const envelope = this.ctx.createGain();
        envelope.gain.setValueAtTime(0, now);
        const attackTime = action === 'blow' ? 0.045 : 0.025;
        envelope.gain.linearRampToValueAtTime(1, now + attackTime);
        // Slight sustain dip
        envelope.gain.linearRampToValueAtTime(0.85, now + attackTime + 0.1);

        // Connect: oscillators → gains → filter → envelope
        osc1.connect(g1); g1.connect(filter);
        osc2.connect(g2); g2.connect(filter);
        osc3.connect(g3); g3.connect(filter);
        noise.connect(gNoise); gNoise.connect(filter);
        filter.connect(envelope);

        // Start
        osc1.start(now);
        osc2.start(now);
        osc3.start(now);
        noise.start(now);

        return {
            output: envelope,
            oscillators: [osc1, osc2, osc3],
            sources: [noise],
            envelope,
            filter
        };
    }

    // ==========================================================
    //  VOICE CREATION — Brass
    // ==========================================================

    _createBrassVoice(freq, velocity, type) {
        const now = this.ctx.currentTime;

        // Instrument-specific parameters
        let effectiveFreq = freq;
        let harmonics, harmonicGains, filterFreq, attackTime;

        switch (type) {
            case 'tuba':
                effectiveFreq = freq / 2; // One octave lower
                harmonics = [1, 2, 3, 4, 5];
                harmonicGains = [0.38, 0.28, 0.14, 0.10, 0.05];
                filterFreq = 1800;
                attackTime = 0.10;
                break;
            case 'french_horn':
                harmonics = [1, 2, 3, 4, 5, 6];
                harmonicGains = [0.30, 0.22, 0.18, 0.12, 0.08, 0.05];
                filterFreq = 3000;
                attackTime = 0.07;
                break;
            default: // trumpet
                harmonics = [1, 2, 3, 4, 5, 6, 7];
                harmonicGains = [0.24, 0.18, 0.16, 0.13, 0.10, 0.07, 0.04];
                filterFreq = 4500;
                attackTime = 0.06;
                break;
        }

        // Envelope with brass-like attack
        const envelope = this.ctx.createGain();
        envelope.gain.setValueAtTime(0, now);
        envelope.gain.linearRampToValueAtTime(velocity * 0.85, now + attackTime);
        envelope.gain.linearRampToValueAtTime(velocity * 0.65, now + attackTime + 0.15);

        // Filter with brass "bloom"
        const filter = this.ctx.createBiquadFilter();
        filter.type = 'lowpass';
        filter.frequency.setValueAtTime(400, now);
        filter.frequency.exponentialRampToValueAtTime(filterFreq, now + attackTime * 1.5);
        filter.Q.value = 1.2;

        // Create harmonic oscillators
        const oscillators = [];
        harmonics.forEach((h, i) => {
            const osc = this.ctx.createOscillator();
            osc.type = 'sawtooth';
            osc.frequency.value = effectiveFreq * h;
            const g = this.ctx.createGain();
            g.gain.value = harmonicGains[i] * velocity;
            osc.connect(g);
            g.connect(filter);
            osc.start(now);
            oscillators.push(osc);
        });

        filter.connect(envelope);

        return {
            output: envelope,
            oscillators,
            sources: [],
            envelope,
            filter
        };
    }

    // ==========================================================
    //  VOICE CREATION — Strings
    // ==========================================================

    _createStringVoice(freq, velocity, type) {
        const now = this.ctx.currentTime;

        const envelope = this.ctx.createGain();

        switch (type) {
            case 'harp':
                // Harp: bright pluck with gentle sustain
                envelope.gain.setValueAtTime(0, now);
                envelope.gain.linearRampToValueAtTime(velocity, now + 0.008);
                envelope.gain.exponentialRampToValueAtTime(velocity * 0.3, now + 0.6);
                envelope.gain.exponentialRampToValueAtTime(0.001, now + 3.5);
                break;
            case 'violin':
                // Violin: bowed, slower attack, sustained
                envelope.gain.setValueAtTime(0, now);
                envelope.gain.linearRampToValueAtTime(velocity * 0.7, now + 0.12);
                break;
            default: // guitar
                // Guitar: sharp pluck, moderate decay
                envelope.gain.setValueAtTime(0, now);
                envelope.gain.linearRampToValueAtTime(velocity, now + 0.004);
                envelope.gain.exponentialRampToValueAtTime(velocity * 0.35, now + 0.35);
                envelope.gain.exponentialRampToValueAtTime(0.001, now + 2.5);
                break;
        }

        // Filter
        const filter = this.ctx.createBiquadFilter();
        filter.type = 'lowpass';
        filter.Q.value = 1;

        switch (type) {
            case 'violin':
                filter.frequency.value = 5500;
                break;
            case 'harp':
                filter.frequency.setValueAtTime(6000, now);
                filter.frequency.exponentialRampToValueAtTime(2000, now + 1.0);
                break;
            default:
                filter.frequency.setValueAtTime(5000, now);
                filter.frequency.exponentialRampToValueAtTime(1500, now + 0.5);
                break;
        }

        // Oscillators
        const oscillators = [];
        const waveform = type === 'violin' ? 'sawtooth' : 'triangle';

        // Fundamental
        const osc1 = this.ctx.createOscillator();
        osc1.type = waveform;
        osc1.frequency.value = freq;
        const g1 = this.ctx.createGain();
        g1.gain.value = 0.35 * velocity;
        osc1.connect(g1); g1.connect(filter);
        osc1.start(now);
        oscillators.push(osc1);

        // Detuned copy for width
        if (type === 'violin') {
            const osc1b = this.ctx.createOscillator();
            osc1b.type = 'sawtooth';
            osc1b.frequency.value = freq * 1.002;
            const g1b = this.ctx.createGain();
            g1b.gain.value = 0.15 * velocity;
            osc1b.connect(g1b); g1b.connect(filter);
            osc1b.start(now);
            oscillators.push(osc1b);
        }

        // Harmonics
        const harmonicMultipliers = type === 'harp' ? [2, 3, 5] : [2, 3, 4];
        harmonicMultipliers.forEach((h, i) => {
            const osc = this.ctx.createOscillator();
            osc.type = 'sine';
            osc.frequency.value = freq * h;
            const g = this.ctx.createGain();
            g.gain.value = (0.18 / (i + 1)) * velocity;
            osc.connect(g); g.connect(filter);
            osc.start(now);
            oscillators.push(osc);
        });

        filter.connect(envelope);

        return {
            output: envelope,
            oscillators,
            sources: [],
            envelope,
            filter
        };
    }

    // ==========================================================
    //  VOICE CREATION — Woodwinds
    // ==========================================================

    _createWoodwindVoice(freq, velocity, type) {
        const now = this.ctx.currentTime;

        const envelope = this.ctx.createGain();
        envelope.gain.setValueAtTime(0, now);

        const oscillators = [];
        const sources = [];

        switch (type) {
            case 'flute': {
                envelope.gain.linearRampToValueAtTime(velocity * 0.65, now + 0.05);

                // Pure sine + airy noise
                const osc = this.ctx.createOscillator();
                osc.type = 'sine';
                osc.frequency.value = freq;
                const g = this.ctx.createGain();
                g.gain.value = 0.32 * velocity;
                osc.connect(g); g.connect(envelope);
                osc.start(now);
                oscillators.push(osc);

                // Octave harmonic
                const osc2 = this.ctx.createOscillator();
                osc2.type = 'sine';
                osc2.frequency.value = freq * 2;
                const g2 = this.ctx.createGain();
                g2.gain.value = 0.08 * velocity;
                osc2.connect(g2); g2.connect(envelope);
                osc2.start(now);
                oscillators.push(osc2);

                // Breathy noise
                const nBuf = this.ctx.createBuffer(1, this.ctx.sampleRate * 0.5, this.ctx.sampleRate);
                const nData = nBuf.getChannelData(0);
                for (let i = 0; i < nData.length; i++) nData[i] = Math.random() * 2 - 1;
                const nSrc = this.ctx.createBufferSource();
                nSrc.buffer = nBuf; nSrc.loop = true;
                const nFilter = this.ctx.createBiquadFilter();
                nFilter.type = 'bandpass';
                nFilter.frequency.value = freq * 1.5;
                nFilter.Q.value = 1.5;
                const nGain = this.ctx.createGain();
                nGain.gain.value = 0.06 * velocity;
                nSrc.connect(nFilter); nFilter.connect(nGain); nGain.connect(envelope);
                nSrc.start(now);
                sources.push(nSrc);
                break;
            }

            case 'clarinet': {
                envelope.gain.linearRampToValueAtTime(velocity * 0.6, now + 0.04);

                // Clarinet: odd harmonics (square-like, but filtered)
                const filter = this.ctx.createBiquadFilter();
                filter.type = 'lowpass';
                filter.frequency.value = 3500;
                filter.Q.value = 0.8;
                filter.connect(envelope);

                [1, 3, 5, 7, 9].forEach((h, i) => {
                    const osc = this.ctx.createOscillator();
                    osc.type = i === 0 ? 'square' : 'sine';
                    osc.frequency.value = freq * h;
                    const g = this.ctx.createGain();
                    g.gain.value = (0.25 / (i + 1)) * velocity;
                    osc.connect(g); g.connect(filter);
                    osc.start(now);
                    oscillators.push(osc);
                });
                break;
            }

            case 'pan_flute': {
                envelope.gain.linearRampToValueAtTime(velocity * 0.6, now + 0.06);

                // Breathy sine + triangle blend
                const osc = this.ctx.createOscillator();
                osc.type = 'sine';
                osc.frequency.value = freq;
                const g = this.ctx.createGain();
                g.gain.value = 0.35 * velocity;
                osc.connect(g); g.connect(envelope);
                osc.start(now);
                oscillators.push(osc);

                const osc2 = this.ctx.createOscillator();
                osc2.type = 'triangle';
                osc2.frequency.value = freq * 2;
                const g2 = this.ctx.createGain();
                g2.gain.value = 0.10 * velocity;
                osc2.connect(g2); g2.connect(envelope);
                osc2.start(now);
                oscillators.push(osc2);

                // More prominent breath noise for pan flute
                const nBuf = this.ctx.createBuffer(1, this.ctx.sampleRate * 0.5, this.ctx.sampleRate);
                const nData = nBuf.getChannelData(0);
                for (let i = 0; i < nData.length; i++) nData[i] = Math.random() * 2 - 1;
                const nSrc = this.ctx.createBufferSource();
                nSrc.buffer = nBuf; nSrc.loop = true;
                const nFilter = this.ctx.createBiquadFilter();
                nFilter.type = 'bandpass';
                nFilter.frequency.value = freq;
                nFilter.Q.value = 0.8;
                const nGain = this.ctx.createGain();
                nGain.gain.value = 0.08 * velocity;
                nSrc.connect(nFilter); nFilter.connect(nGain); nGain.connect(envelope);
                nSrc.start(now);
                sources.push(nSrc);
                break;
            }
        }

        return {
            output: envelope,
            oscillators,
            sources,
            envelope,
            filter: null
        };
    }

    // ==========================================================
    //  CONTROLS
    // ==========================================================

    setKey(key) {
        this.currentKey = key;
        this.stopAll();
    }

    setInstrument(instrument) {
        this.currentInstrument = instrument;
        this.stopAll();
    }

    /**
     * Set EQ band value.
     * @param {string} band - 'low', 'mid', 'high', 'presence'
     * @param {number} value - 0 to 100 (50 = flat)
     */
    setEQ(band, value) {
        if (!this.eqNodes[band]) return;
        const dbValue = (value - 50) * 0.24; // Maps 0-100 to -12..+12 dB
        this.eqNodes[band].gain.linearRampToValueAtTime(
            dbValue,
            this.ctx.currentTime + 0.05
        );
    }

    /**
     * Set effect level.
     * @param {string} name - 'reverb', 'delay', 'chorus'
     * @param {number} value - 0 to 100
     */
    setEffect(name, value) {
        const normalized = value / 100;
        const now = this.ctx.currentTime;

        switch (name) {
            case 'reverb':
                this.reverbGain.gain.linearRampToValueAtTime(normalized * 0.8, now + 0.05);
                break;
            case 'delay':
                this.delayGain.gain.linearRampToValueAtTime(normalized * 0.5, now + 0.05);
                break;
            case 'chorus':
                this.chorusGain.gain.linearRampToValueAtTime(normalized * 0.4, now + 0.05);
                break;
        }
    }

    /**
     * Toggle Safe Mode (aggressive compressor/limiter).
     */
    setSafeMode(enabled) {
        this.safeModeEnabled = enabled;
        const now = this.ctx.currentTime;

        if (enabled) {
            // Aggressive limiting — prevents any harsh sounds
            this.compressor.threshold.linearRampToValueAtTime(-30, now + 0.1);
            this.compressor.ratio.linearRampToValueAtTime(20, now + 0.1);
            this.compressor.knee.linearRampToValueAtTime(10, now + 0.1);
            this.compressor.attack.linearRampToValueAtTime(0.001, now + 0.1);
            this.compressor.release.linearRampToValueAtTime(0.1, now + 0.1);
            this.masterGain.gain.linearRampToValueAtTime(0.5, now + 0.1);
        } else {
            // Light compression — normal operation
            this.compressor.threshold.linearRampToValueAtTime(-6, now + 0.1);
            this.compressor.ratio.linearRampToValueAtTime(3, now + 0.1);
            this.compressor.knee.linearRampToValueAtTime(30, now + 0.1);
            this.compressor.attack.linearRampToValueAtTime(0.003, now + 0.1);
            this.compressor.release.linearRampToValueAtTime(0.25, now + 0.1);
            this.masterGain.gain.linearRampToValueAtTime(0.7, now + 0.1);
        }

        console.log(`[AudioEngine] Safe Mode: ${enabled ? 'ON' : 'OFF'}`);
    }

    /**
     * Toggle auto-tune pitch correction.
     */
    setAutoTune(enabled) {
        this.autoTuneEnabled = enabled;
        console.log(`[AudioEngine] Auto-Tune: ${enabled ? 'ON' : 'OFF'}`);
    }

    /**
     * Set master volume (0-100).
     */
    setMasterVolume(value) {
        if (!this.masterGain) return;
        const v = value / 100;
        this.masterGain.gain.linearRampToValueAtTime(v, this.ctx.currentTime + 0.05);
    }

    /**
     * Resume audio context (needed after user interaction).
     */
    async resume() {
        if (this.ctx && this.ctx.state === 'suspended') {
            await this.ctx.resume();
        }
    }
}

// Export for use
window.AudioEngine = AudioEngine;
