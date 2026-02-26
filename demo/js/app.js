/**
 * Turbo Tuning — Application Controller
 * Handles UI interactions, state management, and connects to the AudioEngine.
 */

(function () {
    'use strict';

    // ==========================================================
    //  STATE
    // ==========================================================

    const state = {
        engine: new AudioEngine(),
        currentAction: 'blow',      // 'blow' or 'draw'
        currentKey: 'C',
        currentInstrument: 'harmonica',
        activeHoles: new Set(),
        keyboardHoles: new Map(),    // Maps keyboard key → hole number
        shiftHeld: false,
    };

    // Keyboard mapping: keys 1-9 → holes 1-9, 0 → hole 10
    const KEY_TO_HOLE = {
        'Digit1': 1, 'Digit2': 2, 'Digit3': 3, 'Digit4': 4, 'Digit5': 5,
        'Digit6': 6, 'Digit7': 7, 'Digit8': 8, 'Digit9': 9, 'Digit0': 10,
        'Numpad1': 1, 'Numpad2': 2, 'Numpad3': 3, 'Numpad4': 4, 'Numpad5': 5,
        'Numpad6': 6, 'Numpad7': 7, 'Numpad8': 8, 'Numpad9': 9, 'Numpad0': 10,
    };

    // Circle of Fifths data
    const CIRCLE_KEYS = AudioEngine.KEYS;
    const RELATIVE_MINORS = AudioEngine.RELATIVE_MINORS;

    // Instrument display names
    const INSTRUMENT_NAMES = {
        harmonica: 'Major Diatonic Harp',
        trumpet: 'Trumpet',
        tuba: 'Tuba',
        french_horn: 'French Horn',
        guitar: 'Guitar',
        harp: 'Harp',
        violin: 'Violin',
        flute: 'Flute',
        clarinet: 'Clarinet',
        pan_flute: 'Pan Flute',
    };

    // ==========================================================
    //  INITIALIZATION
    // ==========================================================

    document.addEventListener('DOMContentLoaded', () => {
        buildHarmonicaHoles();
        buildCircleOfFifths();
        bindEvents();
        updateNoteLabels();
    });

    // ==========================================================
    //  START BUTTON — Audio Context Init
    // ==========================================================

    function initAudio() {
        const overlay = document.getElementById('init-overlay');
        const app = document.getElementById('app');

        state.engine.init().then(() => {
            state.engine.resume();
            overlay.style.opacity = '0';
            overlay.style.pointerEvents = 'none';
            setTimeout(() => {
                overlay.classList.add('hidden');
                app.classList.remove('hidden');
            }, 400);
            console.log('[App] Audio engine initialized');
        });
    }

    // ==========================================================
    //  BUILD UI COMPONENTS
    // ==========================================================

    /**
     * Build the 10 harmonica holes in the DOM.
     */
    function buildHarmonicaHoles() {
        const container = document.getElementById('harmonica-holes');
        container.innerHTML = '';

        for (let i = 1; i <= 10; i++) {
            const hole = document.createElement('div');
            hole.className = 'harmonica-hole';
            hole.dataset.hole = i;
            hole.id = `hole-${i}`;
            hole.setAttribute('role', 'button');
            hole.setAttribute('aria-label', `Hole ${i}`);
            hole.setAttribute('tabindex', '0');

            const label = document.createElement('span');
            label.className = 'hole-note-label';
            label.id = `hole-label-${i}`;
            label.textContent = '';
            hole.appendChild(label);

            container.appendChild(hole);
        }
    }

    /**
     * Build the Circle of Fifths key selector.
     */
    function buildCircleOfFifths() {
        const container = document.getElementById('circle-of-fifths');

        CIRCLE_KEYS.forEach((key, index) => {
            const btn = document.createElement('button');
            btn.className = 'circle-key-btn';
            if (key === state.currentKey) btn.classList.add('active');
            btn.dataset.key = key;
            btn.id = `cof-key-${key.replace('#', 'sharp')}`;
            btn.textContent = key;
            btn.setAttribute('aria-label', `Key of ${key}`);

            // Position around circle
            const angle = (index * 30 - 90) * (Math.PI / 180); // Start from top
            const radius = 42; // % from center
            const cx = 50 + radius * Math.cos(angle);
            const cy = 50 + radius * Math.sin(angle);
            btn.style.left = `${cx}%`;
            btn.style.top = `${cy}%`;

            container.appendChild(btn);
        });
    }

    // ==========================================================
    //  EVENT BINDING
    // ==========================================================

    function bindEvents() {
        // Start button
        document.getElementById('btn-start').addEventListener('click', initAudio);

        // Blow/Draw toggle buttons
        document.getElementById('btn-blow').addEventListener('click', () => setAction('blow'));
        document.getElementById('btn-draw').addEventListener('click', () => setAction('draw'));

        // Harmonica holes — mouse interactions
        const holesContainer = document.getElementById('harmonica-holes');
        holesContainer.addEventListener('mousedown', onHoleMouseDown);
        holesContainer.addEventListener('mouseup', onHoleMouseUp);
        holesContainer.addEventListener('mouseleave', onHoleMouseLeave);
        // Touch support
        holesContainer.addEventListener('touchstart', onHoleTouchStart, { passive: false });
        holesContainer.addEventListener('touchend', onHoleTouchEnd);

        // Circle of Fifths
        document.getElementById('circle-of-fifths').addEventListener('click', onCircleClick);

        // Instrument buttons
        document.querySelectorAll('.instrument-btn').forEach(btn => {
            btn.addEventListener('click', () => selectInstrument(btn.dataset.instrument));
        });

        // EQ sliders
        document.querySelectorAll('.eq-band input[type="range"]').forEach(slider => {
            slider.addEventListener('input', onEQChange);
        });

        // Effect knobs
        document.querySelectorAll('.effect-knob-group input[type="range"]').forEach(slider => {
            slider.addEventListener('input', onEffectChange);
        });

        // Toggles
        document.getElementById('toggle-autotune').addEventListener('change', onAutoTuneToggle);
        document.getElementById('toggle-safemode').addEventListener('change', onSafeModeToggle);

        // Master volume
        document.getElementById('master-volume').addEventListener('input', onMasterVolumeChange);

        // Keyboard events
        document.addEventListener('keydown', onKeyDown);
        document.addEventListener('keyup', onKeyUp);

        // Initialize knob visuals
        document.querySelectorAll('.effect-knob-group input[type="range"]').forEach(slider => {
            updateKnobVisual(slider);
        });
    }

    // ==========================================================
    //  HARMONICA HOLE INTERACTIONS
    // ==========================================================

    function onHoleMouseDown(e) {
        const hole = e.target.closest('.harmonica-hole');
        if (!hole) return;
        e.preventDefault();

        const holeNum = parseInt(hole.dataset.hole);
        const action = e.button === 2 || e.ctrlKey ? getOppositeAction() : state.currentAction;
        playHole(holeNum, action);
    }

    function onHoleMouseUp(e) {
        const hole = e.target.closest('.harmonica-hole');
        if (!hole) return;

        const holeNum = parseInt(hole.dataset.hole);
        stopHole(holeNum);
    }

    function onHoleMouseLeave(e) {
        // Stop all mouse-triggered notes when leaving the container
        state.activeHoles.forEach(holeNum => {
            if (!state.keyboardHoles.has(holeNum)) {
                stopHole(holeNum);
            }
        });
    }

    function onHoleTouchStart(e) {
        e.preventDefault();
        Array.from(e.changedTouches).forEach(touch => {
            const hole = document.elementFromPoint(touch.clientX, touch.clientY);
            if (hole && hole.closest('.harmonica-hole')) {
                const holeNum = parseInt(hole.closest('.harmonica-hole').dataset.hole);
                playHole(holeNum, state.currentAction);
            }
        });
    }

    function onHoleTouchEnd(e) {
        Array.from(e.changedTouches).forEach(touch => {
            const hole = document.elementFromPoint(touch.clientX, touch.clientY);
            if (hole && hole.closest('.harmonica-hole')) {
                const holeNum = parseInt(hole.closest('.harmonica-hole').dataset.hole);
                stopHole(holeNum);
            }
        });
    }

    // ==========================================================
    //  KEYBOARD INTERACTIONS
    // ==========================================================

    function onKeyDown(e) {
        // Don't handle if typing in an input
        if (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA') return;

        // Shift key for draw
        if (e.key === 'Shift' && !state.shiftHeld) {
            state.shiftHeld = true;
            updateActionFromShift();
            return;
        }

        // Space to toggle blow/draw
        if (e.code === 'Space') {
            e.preventDefault();
            toggleAction();
            return;
        }

        // Number keys for holes
        const hole = KEY_TO_HOLE[e.code];
        if (hole && !e.repeat) {
            e.preventDefault();
            const action = state.shiftHeld ? 'draw' : 'blow';
            playHole(hole, action);
            state.keyboardHoles.set(hole, action);
        }
    }

    function onKeyUp(e) {
        if (e.key === 'Shift') {
            state.shiftHeld = false;
            updateActionFromShift();
            return;
        }

        const hole = KEY_TO_HOLE[e.code];
        if (hole) {
            stopHole(hole);
            state.keyboardHoles.delete(hole);
        }
    }

    function updateActionFromShift() {
        // If shift is held, visually indicate draw mode
        if (state.shiftHeld) {
            document.getElementById('btn-draw').classList.add('shift-active');
        } else {
            document.getElementById('btn-draw').classList.remove('shift-active');
        }
    }

    // ==========================================================
    //  PLAY / STOP HOLES
    // ==========================================================

    function playHole(hole, action) {
        const result = state.engine.playNote(hole, action, 0.8);
        if (!result) return;

        state.activeHoles.add(hole);

        // Update UI
        const holeEl = document.getElementById(`hole-${hole}`);
        if (holeEl) {
            holeEl.classList.remove('playing-blow', 'playing-draw');
            holeEl.classList.add(action === 'blow' ? 'playing-blow' : 'playing-draw');
        }

        // Update note display
        updateNoteDisplay(result.note, result.octave, action);

        // Update the hole label
        const label = document.getElementById(`hole-label-${hole}`);
        if (label) {
            label.textContent = result.note;
        }
    }

    function stopHole(hole) {
        // Determine which action was used
        const blowId = `${hole}-blow`;
        const drawId = `${hole}-draw`;

        state.engine.stopNote(hole, 'blow');
        state.engine.stopNote(hole, 'draw');

        state.activeHoles.delete(hole);

        // Update UI
        const holeEl = document.getElementById(`hole-${hole}`);
        if (holeEl) {
            holeEl.classList.remove('playing-blow', 'playing-draw');
        }

        // Clear note display if no notes playing
        if (state.activeHoles.size === 0) {
            clearNoteDisplay();
        }

        // Restore label
        updateNoteLabels();
    }

    // ==========================================================
    //  ACTION TOGGLE (Blow/Draw)
    // ==========================================================

    function setAction(action) {
        state.currentAction = action;
        document.getElementById('btn-blow').classList.toggle('active', action === 'blow');
        document.getElementById('btn-draw').classList.toggle('active', action === 'draw');
        updateNoteLabels();
    }

    function toggleAction() {
        setAction(state.currentAction === 'blow' ? 'draw' : 'blow');
    }

    function getOppositeAction() {
        return state.currentAction === 'blow' ? 'draw' : 'blow';
    }

    // ==========================================================
    //  NOTE DISPLAY
    // ==========================================================

    function updateNoteDisplay(note, octave, action) {
        const noteEl = document.getElementById('note-name');
        const octaveEl = document.getElementById('note-octave');

        noteEl.textContent = note;
        noteEl.className = 'note-display-value ' + (action === 'blow' ? 'blow-active' : 'draw-active');
        octaveEl.textContent = octave;
    }

    function clearNoteDisplay() {
        const noteEl = document.getElementById('note-name');
        const octaveEl = document.getElementById('note-octave');
        noteEl.textContent = '—';
        noteEl.className = 'note-display-value';
        octaveEl.textContent = '';
    }

    /**
     * Update the note labels on each hole based on current key and action.
     */
    function updateNoteLabels() {
        if (!state.engine.isInitialized && !state.engine.ctx) {
            // Before init, show note names for C key
            const blowNotes = AudioEngine.HARMONICA_BLOW;
            const drawNotes = AudioEngine.HARMONICA_DRAW;
            const notes = state.currentAction === 'blow' ? blowNotes : drawNotes;

            for (let i = 1; i <= 10; i++) {
                const label = document.getElementById(`hole-label-${i}`);
                if (label) {
                    const midi = notes[i - 1];
                    const { note } = { note: AudioEngine.NOTE_NAMES[midi % 12] };
                    label.textContent = note;
                }
            }
            return;
        }

        const notes = state.engine.getAllNotes(state.currentAction);
        for (let i = 0; i < 10; i++) {
            const label = document.getElementById(`hole-label-${i + 1}`);
            if (label) {
                const { note } = state.engine.midiToNoteName(notes[i]);
                label.textContent = note;
            }
        }
    }

    // ==========================================================
    //  CIRCLE OF FIFTHS
    // ==========================================================

    function onCircleClick(e) {
        const btn = e.target.closest('.circle-key-btn');
        if (!btn) return;

        const key = btn.dataset.key;
        selectKey(key);
    }

    function selectKey(key) {
        state.currentKey = key;
        state.engine.setKey(key);

        // Update active button
        document.querySelectorAll('.circle-key-btn').forEach(btn => {
            btn.classList.toggle('active', btn.dataset.key === key);
        });

        // Update center display
        document.getElementById('circle-key-name').textContent = key;
        document.getElementById('circle-minor-name').textContent = RELATIVE_MINORS[key] || '';

        // Update header
        document.getElementById('current-key-display').textContent = key;

        // Update note labels
        updateNoteLabels();
    }

    // ==========================================================
    //  INSTRUMENTS
    // ==========================================================

    function selectInstrument(instrument) {
        state.currentInstrument = instrument;
        state.engine.setInstrument(instrument);

        // Update active button
        document.querySelectorAll('.instrument-btn').forEach(btn => {
            btn.classList.toggle('active', btn.dataset.instrument === instrument);
        });

        // Update header display
        document.getElementById('current-instrument-display').textContent =
            INSTRUMENT_NAMES[instrument] || instrument;

        // Update note labels
        updateNoteLabels();
    }

    // ==========================================================
    //  EQ
    // ==========================================================

    function onEQChange(e) {
        const band = e.target.dataset.band;
        const value = parseInt(e.target.value);
        state.engine.setEQ(band, value);

        // Update display
        const dbValue = ((value - 50) * 0.24).toFixed(1);
        const display = document.getElementById(`eq-${band}-val`);
        if (display) {
            display.textContent = `${dbValue > 0 ? '+' : ''}${dbValue}dB`;
        }
    }

    // ==========================================================
    //  EFFECTS
    // ==========================================================

    function onEffectChange(e) {
        const effect = e.target.dataset.effect;
        const value = parseInt(e.target.value);
        state.engine.setEffect(effect, value);

        // Update display
        const display = document.getElementById(`fx-${effect}-val`);
        if (display) {
            display.textContent = `${value}%`;
        }

        // Update knob visual
        updateKnobVisual(e.target);
    }

    function updateKnobVisual(slider) {
        const effect = slider.dataset.effect;
        const value = parseInt(slider.value);
        const knob = document.getElementById(`knob-${effect}`);
        if (!knob) return;

        // Map 0-100 to -135deg to +135deg rotation
        const rotation = (value / 100) * 270 - 135;
        const angle = (value / 100) * 270;

        knob.style.setProperty('--knob-rotation', `${rotation}deg`);
        knob.style.setProperty('--knob-angle', `${angle}deg`);
    }

    // ==========================================================
    //  TOGGLES
    // ==========================================================

    function onAutoTuneToggle(e) {
        state.engine.setAutoTune(e.target.checked);
    }

    function onSafeModeToggle(e) {
        state.engine.setSafeMode(e.target.checked);
    }

    // ==========================================================
    //  MASTER VOLUME
    // ==========================================================

    function onMasterVolumeChange(e) {
        const value = parseInt(e.target.value);
        state.engine.setMasterVolume(value);
        document.getElementById('master-volume-val').textContent = value;
    }

    // ==========================================================
    //  CONTEXT MENU (prevent on harmonica)
    // ==========================================================

    document.addEventListener('contextmenu', (e) => {
        if (e.target.closest('.harmonica-holes')) {
            e.preventDefault();
        }
    });

})();
