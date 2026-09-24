const fs = require('fs');
const files = ['vectors.cpp', ...['src', 'tests'].flatMap(dir => fs.readdirSync(dir).filter(f => /\.(cpp|hpp)$/.test(f)).map(f => dir+'/'+f))];
function read(file) { return fs.readFileSync(file, 'utf8').replace(/\r\n/g, '\n'); }
function write(file, text) { fs.writeFileSync(file, text.trim() + '\n'); }
function bodyBlock(text, marker) {
    const start = text.indexOf(marker);
    if (start < 0) throw Error(marker);
    const open = text.indexOf('{', start);
    let depth = 1, end = open + 1;
    for (; depth; ++end) {
        if (text[end] === '{') ++depth;
        if (text[end] === '}') --depth;
    }
    return text.slice(start, end);
}

// Replace unclear public names consistently in production code and tests.
const names = {getParSystem:'getBodies',getXvel:'getXVelocity',getYvel:'getYVelocity',getRad:'getRadius',setAccel:'setAcceleration',addAccel:'addAcceleration',kick:'advanceVelocity',drift:'advancePosition'};
for (const file of files) {
    let text = read(file);
    for (const [oldName, newName] of Object.entries(names)) {
        text = text.replace(new RegExp('\\b'+oldName+'\\b', 'g'), newName);
    }
    write(file, text);
}

let main = read('vectors.cpp');
main = main.replace(/\bp\b/g, 'initialSystem');
const keyboard = bodyBlock(main, 'if (const auto* key =');
let keyBody = keyboard.slice(keyboard.indexOf('{') + 1, -1).replace('switch (key->code)', 'switch (key)');
main = main.replace(keyboard, `if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                handleKey(key->code, session, camera, frameClock);
            }`);
const follow = bodyBlock(main, 'const auto followSelection =');
const followBody = follow.slice(follow.indexOf('{') + 1, -1);
main = main.replace(follow + ';', '').replace(/followSelection\(\)/g, 'followSelectedBody(session, camera)');
const eventLoop = bodyBlock(main, 'while (const auto event =');
let eventBody = eventLoop.slice(eventLoop.indexOf('{') + 1, -1).replace(/event->/g, 'event.');
main = main.replace(eventLoop, `while (const auto event = window.pollEvent()) {
            handleEvent(*event, window, session, camera, renderer, frameClock);
        }`);
const helpers = `namespace {

void followSelectedBody(const SimulationSession& session, Camera& camera) {
${followBody}
}

void handleKey(sf::Keyboard::Key key, SimulationSession& session,
               Camera& camera, sf::Clock& frameClock) {
${keyBody}
}

void handleEvent(const sf::Event& event, sf::RenderWindow& window,
                 SimulationSession& session, Camera& camera,
                 const Renderer& renderer, sf::Clock& frameClock) {
${eventBody}
}

}

`;
main = main.replace('int main()', helpers + 'int main()');
main = main.replace(/\s*\/\/Create PlanetSystem class\n/, '\n').replace(/\s*\/\/Initialize the values in the vector\n/, '\n');
main = main.replace('// CREATE THE WINDOW AND LOAD THE FONT', '// Load the font before constructing the renderer that uses it.');
main = main.replace(/\s*\/\/_+\n\s*\/\/ MOUSE INPUT\n/, '\n');
main = main.replace(/\s*\/\/Displays the drawing\n/, '\n');
write('vectors.cpp', main);

let rendering = read('src/renderer.cpp');
rendering = rendering.replace(/\bVectorOfLinesV\b/g,'verticalLines').replace(/\bVectorOfLinesH\b/g,'horizontalLines')
    .replace(/\biterateLinesV\b/g,'createVerticalLines').replace(/\biterateLinesH\b/g,'createHorizontalLines')
    .replace(/\bgetLinesV\b/g,'getVerticalLines').replace(/\bgetLinesH\b/g,'getHorizontalLines')
    .replace(/\bGrid g;/,'Grid grid;').replace(/\bg\./g,'grid.')
    .replace(/\bconst Planet& p\b/g,'const Planet& body').replace(/\bp\./g,'body.');
rendering = rendering.replace(/\s*\/\/Nested loop[^\n]*\n/g, '\n').replace(/\s*\/\/Returns the vector as a vector\n/g, '\n');
rendering = rendering.replace(/\s*\/\/_+\n\s*\/\/ GO THROUGH EVERY PLANET IN THE SYSTEM AND DRAW\n/, '\n');
rendering = rendering.replace('//THIS DRAWS THE GRID', '// Update the decorative grid before drawing it.').replace('//Draw all the verticle lines','// Draw the vertical lines.').replace('//Draw all the horizontal lines','// Draw the horizontal lines.');
rendering = rendering.replace('const double worldX = camera.x + (x - 400.0) * SCALE / camera.zoom;\n        const double worldY = camera.y - (y - 300.0) * SCALE / camera.zoom;', 'const auto world = camera.toWorld({x, y});\n        const double worldX = world.x;\n        const double worldY = world.y;');
rendering = rendering.replace('sf::Vector2f normal = length > 0.001f ? sf::Vector2f(-direction.y * radius / length, direction.x * radius / length) : sf::Vector2f(0, radius);', `sf::Vector2f normal(0, radius);

                if (length > 0.001f) {
                    normal = {-direction.y * radius / length, direction.x * radius / length};
                }`);
const draw = bodyBlock(rendering, 'void draw(sf::RenderWindow& window, const SimulationSession& session, const Camera& camera)');
const gridStart = draw.indexOf('        // Update the decorative grid');
const trailsStart = draw.indexOf('        // Draw each body');
const bodiesStart = draw.indexOf('        for (const Planet& body : bodies)');
const selectionStart = draw.indexOf('        if (session.selected');
if ([gridStart,trailsStart,bodiesStart,selectionStart].some(i=>i<0)) throw Error('render sections');
const gridCode = draw.slice(gridStart,trailsStart);
const trailCode = draw.slice(trailsStart,bodiesStart);
const bodyCode = draw.slice(bodiesStart,selectionStart);
const selectCode = draw.slice(selectionStart,draw.indexOf('        statusDisplay.draw'));
const renderMethods = `void drawGrid(sf::RenderWindow& window, const vector<Planet>& bodies, const Camera& camera) {
${gridCode}
    }

    void drawTrails(sf::RenderWindow& window, const vector<Planet>& bodies,
                    const vector<OrbitTrail>& trails, const Camera& camera) {
${trailCode}
    }

    void drawBodies(sf::RenderWindow& window, const vector<Planet>& bodies, const Camera& camera) {
${bodyCode}
    }

    void drawSelection(sf::RenderWindow& window, const SimulationSession& session, const Camera& camera) {
        const auto& bodies = session.system.getBodies();
${selectCode}
    }

    void draw(sf::RenderWindow& window, const SimulationSession& session, const Camera& camera) {
        const auto& bodies = session.system.getBodies();
        drawGrid(window, bodies, camera);
        drawTrails(window, bodies, session.trails, camera);
        drawBodies(window, bodies, camera);
        drawSelection(window, session, camera);
        statusDisplay.draw(window, session);
    }`;
rendering = rendering.replace(draw, renderMethods);
write('src/renderer.cpp', rendering);

let physics = read('src/planet_system.cpp');
const start = physics.indexOf('                const double mass =');
const end = physics.indexOf('                planets[i] = result;');
let mergeBody = physics.slice(start, end).replace('Planet result(', 'return Planet(');
mergeBody = mergeBody.replace(/\bwa\b/g,'firstMassFraction').replace(/\bwb\b/g,'secondMassFraction')
    .replace(/\bra\b/g,'firstScaledRadius').replace(/\brb\b/g,'secondScaledRadius');
mergeBody = mergeBody.replace(/const Planet& appearance =[^]*?\? a : b;/, `const Planet* appearance = &a;

    if (!a.isBlackHole() && (b.isBlackHole() || b.getMass() > a.getMass())) {
        appearance = &b;
    }`);
mergeBody = mergeBody.replace(/appearance\./g, 'appearance->');
physics = physics.slice(0,start) + '                planets[i] = mergeBodies(a, b);\n' + physics.slice(end + '                planets[i] = result;\n'.length);
physics = physics.replace('void PlanetSystem::mergeOverlaps', `namespace {

Planet mergeBodies(const Planet& a, const Planet& b) {
${mergeBody}
}

}

void PlanetSystem::mergeOverlaps`);
physics = physics.replace('// Velocity Verlet: half advanceVelocity, full advancePosition, recompute gravity, half advanceVelocity.', '// Velocity Verlet updates velocity in two half-steps around the position step.');
write('src/planet_system.cpp', physics);

let setup = read('src/setup.cpp');
setup = setup.replace(/\s*\/\/By adding[^\n]*\n/, '\n');
setup = setup.replace('for (int i=0; i<planets.size(); i++) {\n        cout << "\\n" << planets[i].getName() << ":\\n";\n        printInfo(planets[i]);', 'for (const Planet& body : planets) {\n        cout << "\\n" << body.getName() << ":\\n";\n        printInfo(body);');
setup = setup.replace('template <typename T, typename Validator>', '// Read a whole line so invalid input cannot spill into the next field.\ntemplate <typename T, typename Validator>');
write('src/setup.cpp', setup);
