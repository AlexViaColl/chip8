const canvas = document.querySelector("canvas");
const ctx = canvas.getContext("2d");

const wasm = await WebAssembly.instantiateStreaming(
    fetch("chip8.wasm"),
    {env: {rand, print, print_num, fill_rect, memcpy, memset, set_dimensions}}
);
const { memory, game_init, game_input, game_update, get_rom } = wasm.instance.exports;
//console.log(wasm);

const game_memory = new Uint8Array(memory.buffer);
const rom_start = get_rom();

const roms = [
    "15PUZZLE", "BLINKY", "BLITZ", "BRIX", "CONNECT4", "GUESS", "HIDDEN", "INVADERS",
    "KALEID", "MAZE", "MERLIN", "MISSILE", "PONG", "PONG2", "PUZZLE", "SYZYGY",
    "TANK", "test_opcode.ch8", "TETRIS", "TICTAC", "UFO", "VBRIX", "VERS", "WIPEOFF",
];
const selectRoms = document.getElementById("rom");
for (const rom of roms) {
    const option = document.createElement("option");
    option.label = rom;
    option.value = rom;
    selectRoms.appendChild(option);
}
selectRoms.onchange = (e) => {
    (async () => {
        const rom_name = selectRoms.options[selectRoms.selectedIndex].value;
        const rom_bytes = new Uint8Array(await (await fetch(`./ROMS/${rom_name}`)).arrayBuffer());
        for (let i = 0; i < rom_bytes.length; i++) {
            game_memory[rom_start + i] = rom_bytes[i];
        }
        game_init(rom_bytes.length);
    })();
};

const rom_bytes = new Uint8Array(await (await fetch(`./ROMS/${selectRoms.options[selectRoms.selectedIndex].value}`)).arrayBuffer());
for (let i = 0; i < rom_bytes.length; i++) {
    game_memory[rom_start + i] = rom_bytes[i];
}
game_init(rom_bytes.length);

let prevTime = 0;
function step(currTime) {
    const dt = currTime - prevTime;
    prevTime = currTime;

    game_update(dt);

    window.requestAnimationFrame(step);
}

window.requestAnimationFrame(step);

document.addEventListener("keydown", e => {
    game_input(e.key.charCodeAt(0), true);
});
document.addEventListener("keyup", e => {
    game_input(e.key.charCodeAt(0), false);
});

function fill_rect(x, y, w, h, r, g, b) {
    ctx.fillStyle = `rgb(${r}, ${g}, ${b})`;
    ctx.fillRect(x, y, w, h);
}

function print(ptr, len) {
    const buf = new Uint8Array(memory.buffer, ptr, len);
    const str = new TextDecoder().decode(buf);
    console.log(str);
}

function print_num(x) {
    console.log(x);
}

function set_dimensions(w, h) {
    canvas.width = w;
    canvas.height = h;
}

function rand() {
    return Math.floor(Math.random() * 0x7FFF/*RAND_MAX*/);
}
function memcpy(dst, src, size) {
    for (let i = 0; i < size; i++) {
        game_memory[dst + i] = game_memory[src + i];
    }
}
function memset(dst, value, size) {
    for (let i = 0; i < size; i++) {
        game_memory[dst + i] = value;
    }
}
