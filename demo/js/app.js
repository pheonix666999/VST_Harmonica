/* ═══════════════════════════════════════════════
   TURBO HARP – App Logic
   ═══════════════════════════════════════════════ */

// ──── AUDIO CONTEXT ────────────────────────────
let ctx = null;
let masterGain = null;
let reverbNode = null;
let delayNode = null;

function ensureAudio() {
    if (ctx) return;
    ctx = new (window.AudioContext || window.webkitAudioContext)();
    masterGain = ctx.createGain();
    masterGain.gain.value = 0.8;

    // Reverb (convolver impulse)
    reverbNode = ctx.createConvolver();
    makeReverb(1.5, 2.0);

    // Delay
    delayNode = ctx.createDelay(2.0);
    delayNode.delayTime.value = 0.25;

    masterGain.connect(ctx.destination);
}

function makeReverb(duration, decay) {
    if (!ctx) return;
    const sr = ctx.sampleRate;
    const len = sr * duration;
    const buf = ctx.createBuffer(2, len, sr);
    for (let c = 0; c < 2; c++) {
        const d = buf.getChannelData(c);
        for (let i = 0; i < len; i++) {
            d[i] = (Math.random() * 2 - 1) * Math.pow(1 - i / len, decay);
        }
    }
    reverbNode.buffer = buf;
}

// ──── STATE ─────────────────────────────────────
const state = {
    bpm: 120,
    playing: false,
    recording: false,
    seqPlaying: false,
    seqStep: 0,
    seqInterval: null,
    masterVol: 0.8,
    effects: { reverb: true, delay: false, distortion: false, chorus: false, pitch: false, eq: true },
    effectValues: { reverb: 30, delay: 20, distortion: 0, chorus: 0, pitch: 0, eq: 50 },
    trackVols: {},
    trackMutes: {},
    plugins: {},
    activeTab: 'pads',
};

// ──── PAD DEFINITIONS ───────────────────────────
const PADS = [
    { label: 'Kick', emoji: '🥁', color: '#6c63ff', type: 'kick' },
    { label: 'Snare', emoji: '💥', color: '#a78bfa', type: 'snare' },
    { label: 'Hi-Hat', emoji: '🎩', color: '#38bdf8', type: 'hihat' },
    { label: 'Clap', emoji: '👏', color: '#22d3a5', type: 'clap' },
    { label: 'Bass', emoji: '🎸', color: '#f59e0b', type: 'bass' },
    { label: 'Synth', emoji: '🎹', color: '#ec4899', type: 'synth' },
    { label: 'Cowbell', emoji: '🔔', color: '#84cc16', type: 'cowbell' },
    { label: 'Tom', emoji: '🥁', color: '#f97316', type: 'tom' },
    { label: 'Open HH', emoji: '🔊', color: '#06b6d4', type: 'openhat' },
    { label: 'Rim', emoji: '🪘', color: '#8b5cf6', type: 'rim' },
    { label: 'Perc', emoji: '🎶', color: '#d946ef', type: 'perc' },
    { label: 'FX', emoji: '✨', color: '#e11d48', type: 'fx' },
];

// ──── SCALE / NOTES ─────────────────────────────
const SCALES = {
    C: ['C', 'D', 'E', 'F', 'G', 'A', 'B'],
    D: ['D', 'E', 'F#', 'G', 'A', 'B', 'C#'],
    E: ['E', 'F#', 'G#', 'A', 'B', 'C#', 'D#'],
    F: ['F', 'G', 'A', 'Bb', 'C', 'D', 'E'],
    G: ['G', 'A', 'B', 'C', 'D', 'E', 'F#'],
    A: ['A', 'B', 'C#', 'D', 'E', 'F#', 'G#'],
    Bb: ['Bb', 'C', 'D', 'Eb', 'F', 'G', 'A'],
    pentatonic: ['C', 'D', 'E', 'G', 'A'],
    blues: ['C', 'Eb', 'F', 'F#', 'G', 'Bb'],
};

const NOTE_FREQS = {
    'C': 0, 'C#': 1, 'Db': 1, 'D': 2, 'D#': 3, 'Eb': 3, 'E': 4, 'F': 5,
    'F#': 6, 'Gb': 6, 'G': 7, 'G#': 8, 'Ab': 8, 'A': 9, 'A#': 10, 'Bb': 10, 'B': 11,
};

const KEYBOARD_MAP = ['a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l'];

// ──── HELPERS ───────────────────────────────────
function noteToFreq(note, octave) {
    const midi = (NOTE_FREQS[note] ?? 0) + (octave + 1) * 12;
    return 440 * Math.pow(2, (midi - 69) / 12);
}

function showToast(msg) {
    const t = document.getElementById('toast');
    t.textContent = msg;
    t.classList.remove('hidden');
    clearTimeout(t._timeout);
    t._timeout = setTimeout(() => t.classList.add('hidden'), 2500);
}

function setStatus(msg) {
    document.getElementById('status-msg').textContent = msg;
}

// ──── SOUND ENGINE ──────────────────────────────
function playDrum(type) {
    ensureAudio();
    const dest = state.effects.reverb ? reverbNode : ctx.destination;
    const out = ctx.createGain();
    out.gain.value = 0.9 * state.masterVol;
    out.connect(masterGain);

    switch (type) {
        case 'kick': {
            const osc = ctx.createOscillator();
            const g = ctx.createGain();
            osc.frequency.setValueAtTime(150, ctx.currentTime);
            osc.frequency.exponentialRampToValueAtTime(0.01, ctx.currentTime + 0.5);
            g.gain.setValueAtTime(1, ctx.currentTime);
            g.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 0.5);
            osc.connect(g); g.connect(out);
            osc.start(); osc.stop(ctx.currentTime + 0.5);
            break;
        }
        case 'snare': {
            const buf = ctx.createBuffer(1, ctx.sampleRate * 0.1, ctx.sampleRate);
            const d = buf.getChannelData(0);
            for (let i = 0; i < d.length; i++) d[i] = Math.random() * 2 - 1;
            const src = ctx.createBufferSource();
            const bpf = ctx.createBiquadFilter();
            bpf.type = 'bandpass'; bpf.frequency.value = 1500;
            const g = ctx.createGain();
            g.gain.setValueAtTime(1, ctx.currentTime);
            g.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 0.1);
            src.buffer = buf; src.connect(bpf); bpf.connect(g); g.connect(out);
            src.start();
            break;
        }
        case 'hihat': {
            const buf = ctx.createBuffer(1, ctx.sampleRate * 0.05, ctx.sampleRate);
            const d = buf.getChannelData(0);
            for (let i = 0; i < d.length; i++) d[i] = Math.random() * 2 - 1;
            const src = ctx.createBufferSource();
            const hpf = ctx.createBiquadFilter();
            hpf.type = 'highpass'; hpf.frequency.value = 8000;
            const g = ctx.createGain();
            g.gain.setValueAtTime(0.6, ctx.currentTime);
            g.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 0.05);
            src.buffer = buf; src.connect(hpf); hpf.connect(g); g.connect(out);
            src.start();
            break;
        }
        case 'openhat': {
            const buf = ctx.createBuffer(1, ctx.sampleRate * 0.3, ctx.sampleRate);
            const d = buf.getChannelData(0);
            for (let i = 0; i < d.length; i++) d[i] = Math.random() * 2 - 1;
            const src = ctx.createBufferSource();
            const hpf = ctx.createBiquadFilter();
            hpf.type = 'highpass'; hpf.frequency.value = 7000;
            const g = ctx.createGain();
            g.gain.setValueAtTime(0.5, ctx.currentTime);
            g.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 0.25);
            src.buffer = buf; src.connect(hpf); hpf.connect(g); g.connect(out);
            src.start();
            break;
        }
        case 'clap': {
            [0, 0.01, 0.02].forEach(delay => {
                const buf = ctx.createBuffer(1, ctx.sampleRate * 0.1, ctx.sampleRate);
                const d = buf.getChannelData(0);
                for (let i = 0; i < d.length; i++) d[i] = Math.random() * 2 - 1;
                const src = ctx.createBufferSource();
                const bpf = ctx.createBiquadFilter();
                bpf.type = 'bandpass'; bpf.frequency.value = 2000;
                const g = ctx.createGain();
                g.gain.setValueAtTime(0.8, ctx.currentTime + delay);
                g.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + delay + 0.08);
                src.buffer = buf; src.connect(bpf); bpf.connect(g); g.connect(out);
                src.start(ctx.currentTime + delay);
            });
            break;
        }
        case 'bass': playNote('C', 2, 'synth', 0.4); return;
        case 'synth': playNote('E', 3, 'synth', 0.3); return;
        case 'cowbell': {
            const osc1 = ctx.createOscillator(); osc1.frequency.value = 562;
            const osc2 = ctx.createOscillator(); osc2.frequency.value = 845;
            const g = ctx.createGain();
            g.gain.setValueAtTime(0.8, ctx.currentTime);
            g.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 0.4);
            osc1.connect(g); osc2.connect(g); g.connect(out);
            osc1.start(); osc2.start();
            osc1.stop(ctx.currentTime + 0.4); osc2.stop(ctx.currentTime + 0.4);
            break;
        }
        case 'tom': {
            const osc = ctx.createOscillator();
            const g = ctx.createGain();
            osc.frequency.setValueAtTime(200, ctx.currentTime);
            osc.frequency.exponentialRampToValueAtTime(60, ctx.currentTime + 0.3);
            g.gain.setValueAtTime(0.9, ctx.currentTime);
            g.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 0.3);
            osc.connect(g); g.connect(out);
            osc.start(); osc.stop(ctx.currentTime + 0.3);
            break;
        }
        case 'rim': {
            const buf = ctx.createBuffer(1, ctx.sampleRate * 0.03, ctx.sampleRate);
            const d = buf.getChannelData(0);
            for (let i = 0; i < d.length; i++) d[i] = Math.random() * 2 - 1;
            const src = ctx.createBufferSource();
            const bpf = ctx.createBiquadFilter();
            bpf.type = 'bandpass'; bpf.frequency.value = 3000;
            const g = ctx.createGain();
            g.gain.setValueAtTime(0.7, ctx.currentTime);
            g.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 0.03);
            src.buffer = buf; src.connect(bpf); bpf.connect(g); g.connect(out);
            src.start();
            break;
        }
        case 'perc': playNote('G', 4, 'harmonica', 0.2); return;
        case 'fx': {
            // Riser sweep
            const osc = ctx.createOscillator();
            const g = ctx.createGain();
            osc.type = 'sawtooth';
            osc.frequency.setValueAtTime(80, ctx.currentTime);
            osc.frequency.exponentialRampToValueAtTime(1200, ctx.currentTime + 0.5);
            g.gain.setValueAtTime(0.3, ctx.currentTime);
            g.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 0.5);
            osc.connect(g); g.connect(out);
            osc.start(); osc.stop(ctx.currentTime + 0.5);
            break;
        }
    }
}

function playNote(noteName, octave, voice, duration = 0.6) {
    ensureAudio();
    const freq = noteToFreq(noteName, octave);
    const out = ctx.createGain();
    out.gain.value = 0.7 * state.masterVol;
    out.connect(masterGain);

    switch (voice) {
        case 'harmonica': {
            // Multiple detuned waves for harmonica-like sound
            [0, 4, -4, 8].forEach(detune => {
                const osc = ctx.createOscillator();
                osc.type = 'sawtooth';
                osc.frequency.value = freq;
                osc.detune.value = detune;
                const lpf = ctx.createBiquadFilter();
                lpf.type = 'lowpass'; lpf.frequency.value = 2000;
                const g = ctx.createGain();
                g.gain.setValueAtTime(0, ctx.currentTime);
                g.gain.linearRampToValueAtTime(0.18, ctx.currentTime + 0.02);
                g.gain.setValueAtTime(0.18, ctx.currentTime + duration - 0.05);
                g.gain.linearRampToValueAtTime(0, ctx.currentTime + duration);
                osc.connect(lpf); lpf.connect(g); g.connect(out);
                osc.start(); osc.stop(ctx.currentTime + duration);
            });
            break;
        }
        case 'piano': {
            const osc = ctx.createOscillator();
            osc.type = 'triangle'; osc.frequency.value = freq;
            const osc2 = ctx.createOscillator();
            osc2.type = 'sine'; osc2.frequency.value = freq * 2;
            const g = ctx.createGain();
            g.gain.setValueAtTime(0.8, ctx.currentTime);
            g.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + duration);
            osc.connect(g); osc2.connect(g); g.connect(out);
            osc.start(); osc2.start();
            osc.stop(ctx.currentTime + duration); osc2.stop(ctx.currentTime + duration);
            break;
        }
        case 'synth': {
            const osc = ctx.createOscillator();
            osc.type = 'square'; osc.frequency.value = freq;
            const lpf = ctx.createBiquadFilter();
            lpf.type = 'lowpass'; lpf.frequency.setValueAtTime(2000, ctx.currentTime);
            lpf.frequency.exponentialRampToValueAtTime(200, ctx.currentTime + duration);
            const g = ctx.createGain();
            g.gain.setValueAtTime(0.5, ctx.currentTime);
            g.gain.linearRampToValueAtTime(0.001, ctx.currentTime + duration);
            osc.connect(lpf); lpf.connect(g); g.connect(out);
            osc.start(); osc.stop(ctx.currentTime + duration);
            break;
        }
        case 'organ': {
            [1, 2, 4, 0.5].forEach((mul, i) => {
                const osc = ctx.createOscillator();
                osc.type = 'sine'; osc.frequency.value = freq * mul;
                const g = ctx.createGain();
                const vol = [0.4, 0.2, 0.15, 0.1][i];
                g.gain.setValueAtTime(vol, ctx.currentTime);
                g.gain.linearRampToValueAtTime(vol * 0.8, ctx.currentTime + duration);
                g.gain.linearRampToValueAtTime(0, ctx.currentTime + duration + 0.05);
                osc.connect(g); g.connect(out);
                osc.start(); osc.stop(ctx.currentTime + duration + 0.05);
            });
            break;
        }
    }
}

// ──── BUILD PADS UI ─────────────────────────────
function buildPads() {
    const grid = document.getElementById('pads-grid');
    grid.innerHTML = '';
    PADS.forEach((pad, i) => {
        const btn = document.createElement('button');
        btn.className = 'pad';
        btn.id = `pad-${pad.type}`;
        btn.style.background = `linear-gradient(145deg, ${pad.color}dd, ${pad.color}88)`;
        btn.innerHTML = `<span>${pad.emoji}</span><span class="pad-label">${pad.label}</span>`;
        btn.addEventListener('pointerdown', () => {
            ensureAudio();
            playDrum(pad.type);
            btn.classList.add('active');
            setTimeout(() => btn.classList.remove('active'), 120);
            setStatus(`🥁 ${pad.label}!`);
        });
        grid.appendChild(btn);
    });
}

// ──── SEQUENCER ─────────────────────────────────
const SEQ_STEPS = 16;
const SEQ_ROWS = ['kick', 'snare', 'hihat', 'clap'];
const seqGrid = {}; // { type: [bool x16] }

function buildSequencer() {
    const seq = document.getElementById('sequencer');
    seq.innerHTML = '';
    SEQ_ROWS.forEach(type => {
        seqGrid[type] = new Array(SEQ_STEPS).fill(false);
        const row = document.createElement('div');
        row.className = 'seq-row';
        const label = document.createElement('div');
        label.className = 'seq-row-label';
        label.textContent = PADS.find(p => p.type === type)?.label || type;
        row.appendChild(label);
        for (let i = 0; i < SEQ_STEPS; i++) {
            const btn = document.createElement('button');
            btn.className = 'seq-btn';
            btn.dataset.type = type;
            btn.dataset.step = i;
            btn.addEventListener('click', () => {
                seqGrid[type][i] = !seqGrid[type][i];
                btn.classList.toggle('on', seqGrid[type][i]);
            });
            row.appendChild(btn);
        }
        seq.appendChild(row);
    });
}

function stepSequencer() {
    const step = state.seqStep;
    // Clear old highlights
    document.querySelectorAll('.seq-btn.step-active').forEach(b => b.classList.remove('step-active'));
    // Highlight current step
    document.querySelectorAll(`.seq-btn[data-step="${step}"]`).forEach(b => b.classList.add('step-active'));
    // Play sounds
    SEQ_ROWS.forEach(type => {
        if (seqGrid[type][step]) {
            playDrum(type);
        }
    });
    state.seqStep = (step + 1) % SEQ_STEPS;
}

document.getElementById('seq-play').addEventListener('click', () => {
    ensureAudio();
    if (state.seqPlaying) {
        clearInterval(state.seqInterval); state.seqPlaying = false;
        state.seqStep = 0;
        document.querySelectorAll('.seq-btn.step-active').forEach(b => b.classList.remove('step-active'));
        document.getElementById('seq-play').textContent = '▶ Play Loop';
        setStatus('Sequencer stopped');
    } else {
        state.seqPlaying = true;
        const interval = (60 / state.bpm / 4) * 1000;
        state.seqInterval = setInterval(stepSequencer, interval);
        document.getElementById('seq-play').textContent = '⏹ Stop Loop';
        setStatus('Sequencer playing 🔥');
    }
});

document.getElementById('seq-clear').addEventListener('click', () => {
    SEQ_ROWS.forEach(type => {
        seqGrid[type] = new Array(SEQ_STEPS).fill(false);
    });
    document.querySelectorAll('.seq-btn').forEach(b => b.classList.remove('on'));
    showToast('Sequencer cleared!');
});

// ──── BUILD HARP KEYS ────────────────────────────
let activeKeyNodes = {};

function buildHarpKeys() {
    const container = document.getElementById('harp-keys');
    container.innerHTML = '';
    const key = document.getElementById('key-select').value;
    const notes = SCALES[key] || SCALES['C'];
    const shortcuts = KEYBOARD_MAP.slice(0, notes.length);

    notes.forEach((note, i) => {
        const btn = document.createElement('button');
        btn.className = 'harp-key';
        btn.id = `hkey-${i}`;
        btn.innerHTML = `<span class="key-note">${note}</span><span class="key-shortcut">${shortcuts[i] || ''}</span>`;

        const press = () => {
            ensureAudio();
            const octave = parseInt(document.getElementById('octave-select').value);
            const voice = document.getElementById('voice-select').value;
            playNote(note, octave, voice);
            btn.classList.add('pressed');
            setStatus(`🎵 Playing ${note}${octave}`);
        };
        const release = () => btn.classList.remove('pressed');

        btn.addEventListener('pointerdown', press);
        btn.addEventListener('pointerup', release);
        btn.addEventListener('pointerleave', release);
        btn.dataset.note = note;
        btn.dataset.index = i;
        container.appendChild(btn);
    });
}

// Keyboard support
const pressedKeys = new Set();
document.addEventListener('keydown', e => {
    if (state.activeTab !== 'keys') return;
    if (pressedKeys.has(e.key)) return;
    const idx = KEYBOARD_MAP.indexOf(e.key.toLowerCase());
    if (idx < 0) return;
    pressedKeys.add(e.key);
    const btn = document.getElementById(`hkey-${idx}`);
    if (btn) btn.dispatchEvent(new Event('pointerdown'));
});
document.addEventListener('keyup', e => {
    if (state.activeTab !== 'keys') return;
    const idx = KEYBOARD_MAP.indexOf(e.key.toLowerCase());
    pressedKeys.delete(e.key);
    if (idx < 0) return;
    const btn = document.getElementById(`hkey-${idx}`);
    if (btn) btn.dispatchEvent(new Event('pointerup'));
});

// Chords
const CHORD_INTERVALS = { I: [0, 4, 7], IV: [3, 7, 10], V: [7, 11, 14], vi: [9, 12, 16] };

document.querySelectorAll('.chord-btn').forEach(btn => {
    btn.addEventListener('pointerdown', () => {
        ensureAudio();
        const chord = btn.dataset.chord;
        const key = document.getElementById('key-select').value;
        const octave = parseInt(document.getElementById('octave-select').value);
        const voice = document.getElementById('voice-select').value;
        const notes = SCALES[key] || SCALES['C'];
        const intervals = CHORD_INTERVALS[chord] || CHORD_INTERVALS.I;
        intervals.forEach(interval => {
            const note = notes[interval % notes.length];
            const oct = octave + Math.floor(interval / 7);
            playNote(note, oct, voice, 0.8);
        });
        showToast(`🎵 ${key} ${chord} chord`);
    });
});

document.getElementById('key-select').addEventListener('change', buildHarpKeys);
document.getElementById('octave-select').addEventListener('change', buildHarpKeys);

// ──── MIXER ─────────────────────────────────────
const MIXER_TRACKS = [
    { id: 'keys', name: 'Harp', icon: '🎵' },
    { id: 'drums', name: 'Drums', icon: '🥁' },
    { id: 'bass', name: 'Bass', icon: '🎸' },
    { id: 'synth', name: 'Synth', icon: '🎹' },
    { id: 'fx', name: 'FX', icon: '✨' },
    { id: 'master', name: 'Master', icon: '🎚' },
];

function buildMixer() {
    const container = document.getElementById('mixer-tracks');
    container.innerHTML = '';
    MIXER_TRACKS.forEach(track => {
        state.trackVols[track.id] = 80;
        state.trackMutes[track.id] = false;

        const div = document.createElement('div');
        div.className = 'mixer-track';
        div.innerHTML = `
      <div class="track-name">${track.name}</div>
      <div class="track-icon">${track.icon}</div>
      <input type="range" class="track-fader" id="fader-${track.id}" min="0" max="100" value="80" />
      <span class="track-vol-label" id="vol-${track.id}">80%</span>
      <button class="track-mute" id="mute-${track.id}">M</button>
    `;
        container.appendChild(div);

        document.getElementById(`fader-${track.id}`).addEventListener('input', e => {
            const val = e.target.value;
            state.trackVols[track.id] = val;
            document.getElementById(`vol-${track.id}`).textContent = val + '%';
            if (track.id === 'master' || track.id === 'keys') {
                if (masterGain) masterGain.gain.value = (val / 100) * state.masterVol;
            }
        });

        document.getElementById(`mute-${track.id}`).addEventListener('click', e => {
            state.trackMutes[track.id] = !state.trackMutes[track.id];
            e.target.classList.toggle('muted', state.trackMutes[track.id]);
            e.target.textContent = state.trackMutes[track.id] ? 'MUTED' : 'M';
            showToast(state.trackMutes[track.id] ? `🔇 ${track.name} muted` : `🔊 ${track.name} unmuted`);
        });
    });
}

// Master volume
document.getElementById('master-vol').addEventListener('input', e => {
    state.masterVol = e.target.value / 100;
    document.getElementById('master-vol-val').textContent = e.target.value + '%';
    if (masterGain) masterGain.gain.value = state.masterVol;
});

// ──── EFFECTS ───────────────────────────────────
const FX_UNITS = ['reverb', 'delay', 'distortion', 'chorus', 'pitch', 'eq'];

FX_UNITS.forEach(fx => {
    const knob = document.getElementById(`knob-${fx}`);
    const valEl = document.getElementById(`val-${fx}`);
    const toggle = document.querySelector(`[data-fx="${fx}"]`);

    if (knob) {
        knob.addEventListener('input', e => {
            state.effectValues[fx] = parseFloat(e.target.value);
            if (fx === 'pitch') {
                valEl.textContent = e.target.value > 0 ? `+${e.target.value} st` : `${e.target.value} st`;
            } else {
                valEl.textContent = e.target.value + '%';
            }
            // Apply reverb wetness
            if (fx === 'reverb' && reverbNode) {
                makeReverb(0.5 + (state.effectValues.reverb / 100) * 3, 1.5);
            }
        });
    }

    if (toggle) {
        toggle.addEventListener('click', () => {
            state.effects[fx] = !state.effects[fx];
            toggle.classList.toggle('active', state.effects[fx]);
            toggle.textContent = state.effects[fx] ? 'ON' : 'OFF';
            showToast(`${fx} ${state.effects[fx] ? '✅ ON' : '❌ OFF'}`);
        });
    }
});

// ──── PLUGINS DATA ──────────────────────────────
const PLUGIN_DATA = [
    { name: 'Jazz Harmonica Pro', desc: 'Professional jazz harmonica with 8 tone holes', icon: '🎵', cat: 'instruments', size: '12 MB', price: 'free' },
    { name: 'Deep Bass Synth', desc: 'Punchy sub bass for hip-hop & trap beats', icon: '🎸', cat: 'instruments', size: '8 MB', price: 'free' },
    { name: 'Trap Drum Kit', desc: '808s, claps, hi-hats. All you need for fire beats', icon: '🥁', cat: 'drums', size: '45 MB', price: 'free' },
    { name: 'Lo-Fi Piano', desc: 'Warm, vintage piano samples with tape saturation', icon: '🎹', cat: 'instruments', size: '30 MB', price: 'free' },
    { name: 'Dreamy Reverb', desc: 'Lush, spacious reverb inspired by real springs', icon: '🏟', cat: 'effects', size: '2 MB', price: 'free' },
    { name: 'Vintage Tape Echo', desc: 'Classic tape delay with wow & flutter', icon: '📼', cat: 'effects', size: '3 MB', price: 'free' },
    { name: 'Chill Lo-Fi Loops', desc: '100+ lo-fi loops in all tempos and keys', icon: '🌙', cat: 'loops', size: '120 MB', price: 'free' },
    { name: 'EDM Drop Kit', desc: 'Risers, drops, and FX for your next EDM banger', icon: '⚡', cat: 'loops', size: '55 MB', price: 'free' },
    { name: 'Gospel Choir', desc: 'Full SATB choir runs, chords, and backgrounds', icon: '🎤', cat: 'instruments', size: '200 MB', price: 'free' },
    { name: 'Vinyl Crackle', desc: 'Authentic vinyl noise and crackle textures', icon: '📀', cat: 'effects', size: '5 MB', price: 'free' },
    { name: 'World Percussion', desc: 'Djembe, congas, talking drum, cajon & more', icon: '🪘', cat: 'drums', size: '80 MB', price: 'free' },
    { name: 'Future Bass Chords', desc: 'Supersaw chord stabs and arpeggios', icon: '🌊', cat: 'instruments', size: '20 MB', price: 'free' },
];

function buildPlugins(filter = 'all', search = '') {
    const list = document.getElementById('plugins-list');
    list.innerHTML = '';
    const filtered = PLUGIN_DATA.filter(p => {
        const matchCat = filter === 'all' || p.cat === filter;
        const matchSearch = !search || p.name.toLowerCase().includes(search.toLowerCase()) || p.desc.toLowerCase().includes(search.toLowerCase());
        return matchCat && matchSearch;
    });
    if (!filtered.length) {
        list.innerHTML = '<p style="color:var(--text-dim);text-align:center;padding:20px">No plugins found 😕</p>';
        return;
    }
    filtered.forEach(p => {
        const installed = state.plugins[p.name] === 'installed';
        const card = document.createElement('div');
        card.className = 'plugin-card';
        card.innerHTML = `
      <div class="plugin-icon">${p.icon}</div>
      <div class="plugin-info">
        <div class="plugin-name">${p.name}</div>
        <div class="plugin-desc">${p.desc}</div>
        <div class="plugin-meta">
          <span class="plugin-tag">${p.cat}</span>
          <span class="plugin-tag free">${p.price}</span>
          <span class="plugin-size">${p.size}</span>
        </div>
      </div>
      <button class="plugin-dl-btn ${installed ? 'installed' : 'get'}" data-name="${p.name}">
        ${installed ? '✅ Installed' : '⬇ Get'}
      </button>
    `;
        const btn = card.querySelector('.plugin-dl-btn');
        if (!installed) {
            btn.addEventListener('click', () => downloadPlugin(p, btn));
        }
        list.appendChild(card);
    });
}

function downloadPlugin(plugin, btn) {
    if (state.plugins[plugin.name]) return;
    btn.className = 'plugin-dl-btn downloading';
    btn.textContent = '⏳ Loading…';
    setStatus(`⬇ Downloading ${plugin.name}…`);

    // Simulate download
    const secs = 1 + Math.random() * 2;
    setTimeout(() => {
        state.plugins[plugin.name] = 'installed';
        btn.className = 'plugin-dl-btn installed';
        btn.textContent = '✅ Installed';
        showToast(`${plugin.icon} ${plugin.name} installed!`);
        setStatus(`✅ ${plugin.name} ready to use!`);
    }, secs * 1000);
}

// Category filter
document.querySelectorAll('.cat-btn').forEach(btn => {
    btn.addEventListener('click', () => {
        document.querySelectorAll('.cat-btn').forEach(b => b.classList.remove('active'));
        btn.classList.add('active');
        buildPlugins(btn.dataset.cat, document.getElementById('plugin-search').value);
    });
});

document.getElementById('plugin-search').addEventListener('input', e => {
    const activeCat = document.querySelector('.cat-btn.active')?.dataset.cat || 'all';
    buildPlugins(activeCat, e.target.value);
});

// ──── TRANSPORT ──────────────────────────────────
document.getElementById('bpm-slider').addEventListener('input', e => {
    state.bpm = parseInt(e.target.value);
    document.getElementById('bpm-val').textContent = state.bpm;
    // Update sequencer interval if playing
    if (state.seqPlaying) {
        clearInterval(state.seqInterval);
        const interval = (60 / state.bpm / 4) * 1000;
        state.seqInterval = setInterval(stepSequencer, interval);
    }
});

document.getElementById('btn-play').addEventListener('click', () => {
    ensureAudio();
    state.playing = !state.playing;
    document.getElementById('btn-play').classList.toggle('playing', state.playing);
    document.getElementById('btn-play').textContent = state.playing ? '⏸' : '▶';
    setStatus(state.playing ? '▶ Playing...' : '⏸ Stopped');
});

document.getElementById('btn-rec').addEventListener('click', () => {
    ensureAudio();
    state.recording = !state.recording;
    document.getElementById('btn-rec').classList.toggle('recording', state.recording);
    setStatus(state.recording ? '⏺ Recording... Play something!' : '⏹ Recording stopped');
    if (state.recording) showToast('⏺ Recording! Play the pads or keys.');
});

document.getElementById('btn-rewind').addEventListener('click', () => {
    state.seqStep = 0;
    setStatus('⏮ Rewound to start');
});

// ──── TABS ──────────────────────────────────────
document.querySelectorAll('.nav-btn').forEach(btn => {
    btn.addEventListener('click', () => {
        const tab = btn.dataset.tab;
        state.activeTab = tab;
        document.querySelectorAll('.nav-btn').forEach(b => b.classList.remove('active'));
        document.querySelectorAll('.tab-section').forEach(s => s.classList.remove('active'));
        btn.classList.add('active');
        document.getElementById(`tab-${tab}`).classList.add('active');
        setStatus(`📂 ${btn.textContent.trim()}`);
    });
});

// ──── VISUALIZER ─────────────────────────────────
const canvas = document.getElementById('visualizer');
const canvasCtx = canvas.getContext('2d');
let analyser = null;

function initVisualizer() {
    if (!ctx || analyser) return;
    analyser = ctx.createAnalyser();
    analyser.fftSize = 256;
    masterGain.connect(analyser);
    requestAnimationFrame(drawVisualizer);
}

function drawVisualizer() {
    requestAnimationFrame(drawVisualizer);
    if (!analyser) {
        // Draw idle animation
        canvasCtx.clearRect(0, 0, canvas.width, canvas.height);
        const t = Date.now() / 1000;
        const bars = 40;
        const w = canvas.width / bars;
        for (let i = 0; i < bars; i++) {
            const h = 4 + Math.sin(t * 2 + i * 0.5) * 8 + Math.sin(t * 3 + i * 0.3) * 6;
            const grad = canvasCtx.createLinearGradient(0, 60, 0, 60 - h);
            grad.addColorStop(0, '#6c63ff');
            grad.addColorStop(1, '#38bdf8');
            canvasCtx.fillStyle = grad;
            canvasCtx.beginPath();
            canvasCtx.roundRect(i * w + 1, 60 - h, w - 2, h, 2);
            canvasCtx.fill();
        }
        return;
    }

    const buf = new Uint8Array(analyser.frequencyBinCount);
    analyser.getByteFrequencyData(buf);
    const W = canvas.width, H = canvas.height;
    canvasCtx.clearRect(0, 0, W, H);
    const bw = W / buf.length;
    for (let i = 0; i < buf.length; i++) {
        const h = (buf[i] / 255) * H;
        const hue = 220 + (i / buf.length) * 120;
        canvasCtx.fillStyle = `hsl(${hue}, 80%, 60%)`;
        canvasCtx.beginPath();
        canvasCtx.roundRect(i * bw, H - h, bw - 1, h, 1);
        canvasCtx.fill();
    }
}

// Start idle visualizer immediately
drawVisualizer();

// Hook audio init to init visualizer too
const origEnsure = ensureAudio;
function ensureAudio() {
    if (ctx) return;
    origEnsure();
    initVisualizer();
}

// Wait—redefine properly:
(function () {
    let inited = false;
    window.ensureAudio = function () {
        if (inited) return;
        inited = true;
        ctx = new (window.AudioContext || window.webkitAudioContext)();
        masterGain = ctx.createGain();
        masterGain.gain.value = state.masterVol;
        makeReverb(1.5, 2.0);
        reverbNode = ctx.createConvolver();
        makeReverb(1.5, 2.0);
        masterGain.connect(ctx.destination);
        // Visualizer
        analyser = ctx.createAnalyser();
        analyser.fftSize = 256;
        masterGain.connect(analyser);
        setStatus('🎵 Audio ready! Let\'s make music!');
    };
})();

// ──── PWA INSTALL ───────────────────────────────
let deferredPrompt = null;
window.addEventListener('beforeinstallprompt', e => {
    e.preventDefault();
    deferredPrompt = e;
    document.getElementById('install-banner').classList.remove('hidden');
});

document.getElementById('install-btn').addEventListener('click', async () => {
    if (!deferredPrompt) return;
    deferredPrompt.prompt();
    const { outcome } = await deferredPrompt.userChoice;
    if (outcome === 'accepted') showToast('🎉 Turbo Harp installed!');
    deferredPrompt = null;
    document.getElementById('install-banner').classList.add('hidden');
});

document.getElementById('install-dismiss').addEventListener('click', () => {
    document.getElementById('install-banner').classList.add('hidden');
});

// Service Worker
if ('serviceWorker' in navigator) {
    navigator.serviceWorker.register('sw.js').catch(() => { });
}

// ──── INIT ──────────────────────────────────────
buildPads();
buildSequencer();
buildHarpKeys();
buildMixer();
buildPlugins();
setStatus('Ready to make bangers 🎵');
