-- Asheteroids - a self-contained Asteroids game
-- Uses only the APIs implemented in the Playdate runtime

local gfx = playdate.graphics
local W, H = 400, 240

local ship = {
    x = W/2, y = H/2,
    dx = 0, dy = 0,
    angle = 0,
    thrusting = false,
    alive = true,
    invuln = 120,
}

local bullets = {}
local asteroids = {}
local particles = {}
local score = 0
local lives = 3
local level = 1
local gameOver = false

local function spawnAsteroids(n)
    for i = 1, n do
        local x, y
        repeat
            x = math.random(W)
            y = math.random(H)
        until ((x - ship.x)^2 + (y - ship.y)^2) > 80^2
        local angle = math.random() * 2 * math.pi
        local speed = 0.5 + math.random() * 1.0 + level * 0.1
        table.insert(asteroids, {
            x = x, y = y,
            dx = math.cos(angle) * speed,
            dy = math.sin(angle) * speed,
            radius = 12,
            angle = 0,
            da = (math.random() - 0.5) * 4,
            verts = {5, 0, 3, -3, -2, -4, -4, -1, -3, 2, 0, 4, 3, 3, 5, 0},
        })
    end
end

local function startLevel()
    asteroids = {}
    bullets = {}
    spawnAsteroids(3 + level)
    ship.x = W/2
    ship.y = H/2
    ship.dx = 0
    ship.dy = 0
    ship.angle = 0
    ship.invuln = 120
end

local function explode(x, y, count)
    for i = 1, count do
        local a = math.random() * 2 * math.pi
        local s = 0.5 + math.random() * 2.0
        table.insert(particles, {
            x = x, y = y,
            dx = math.cos(a) * s,
            dy = math.sin(a) * s,
            life = 20 + math.random(20),
        })
    end
end

local function wrap(obj)
    if obj.x < 0 then obj.x = obj.x + W
    elseif obj.x >= W then obj.x = obj.x - W end
    if obj.y < 0 then obj.y = obj.y + H
    elseif obj.y >= H then obj.y = obj.y - H end
end

local function drawShip(cx, cy, angle, thrusting)
    local cos_a = math.cos(math.rad(angle))
    local sin_a = math.sin(math.rad(angle))
    local function tx(x, y) return cx + x * cos_a - y * sin_a end
    local function ty(x, y) return cy + x * sin_a + y * cos_a end
    gfx.drawLine(tx(6,0), ty(6,0), tx(-4,3), ty(-4,3), 1)
    gfx.drawLine(tx(-4,3), ty(-4,3), tx(-2,0), ty(-2,0), 1)
    gfx.drawLine(tx(-2,0), ty(-2,0), tx(-4,-3), ty(-4,-3), 1)
    gfx.drawLine(tx(-4,-3), ty(-4,-3), tx(6,0), ty(6,0), 1)
    if thrusting then
        local flicker = math.random(2, 5)
        gfx.drawLine(tx(-4,3), ty(-4,3), tx(-4-flicker,0), ty(-4-flicker,0), 1)
        gfx.drawLine(tx(-4,-3), ty(-4,-3), tx(-4-flicker,0), ty(-4-flicker,0), 1)
    end
end

local function drawAsteroid(a)
    local cos_a = math.cos(math.rad(a.angle))
    local sin_a = math.sin(math.rad(a.angle))
    local r = a.radius
    local prev_x, prev_y
    local first_x, first_y
    for i = 1, #a.verts, 2 do
        local px = (a.verts[i] / 5) * r
        local py = (a.verts[i+1] / 5) * r
        local sx = a.x + px * cos_a - py * sin_a
        local sy = a.y + px * sin_a + py * cos_a
        if prev_x then
            gfx.drawLine(prev_x, prev_y, sx, sy, 1)
        else
            first_x, first_y = sx, sy
        end
        prev_x, prev_y = sx, sy
    end
    gfx.drawLine(prev_x, prev_y, first_x, first_y, 1)
end

local function updateShip()
    if not ship.alive then return end
    local current, pushed, released = playdate.getButtonState()

    if current & playdate.kButtonLeft ~= 0 then ship.angle = ship.angle - 4 end
    if current & playdate.kButtonRight ~= 0 then ship.angle = ship.angle + 4 end

    ship.thrusting = (current & playdate.kButtonUp ~= 0)

    if ship.thrusting then
        local thrust = 0.15
        ship.dx = ship.dx + math.cos(math.rad(ship.angle)) * thrust
        ship.dy = ship.dy + math.sin(math.rad(ship.angle)) * thrust
        local maxspeed = 5
        local speed = math.sqrt(ship.dx^2 + ship.dy^2)
        if speed > maxspeed then
            ship.dx = ship.dx * maxspeed / speed
            ship.dy = ship.dy * maxspeed / speed
        end
    end

    ship.x = ship.x + ship.dx
    ship.y = ship.y + ship.dy
    ship.dx = ship.dx * 0.99
    ship.dy = ship.dy * 0.99
    wrap(ship)

    if pushed & playdate.kButtonA ~= 0 then
        if #bullets < 4 then
            table.insert(bullets, {
                x = ship.x + math.cos(math.rad(ship.angle)) * 6,
                y = ship.y + math.sin(math.rad(ship.angle)) * 6,
                dx = ship.dx + math.cos(math.rad(ship.angle)) * 6,
                dy = ship.dy + math.sin(math.rad(ship.angle)) * 6,
                life = 60,
            })
        end
    end

    if ship.invuln > 0 then ship.invuln = ship.invuln - 1 end
end

local function updateBullets()
    for i = #bullets, 1, -1 do
        local b = bullets[i]
        b.x = b.x + b.dx
        b.y = b.y + b.dy
        b.life = b.life - 1
        wrap(b)
        if b.life <= 0 then
            table.remove(bullets, i)
        end
    end
end

local function updateAsteroids()
    for i = 1, #asteroids do
        local a = asteroids[i]
        a.x = a.x + a.dx
        a.y = a.y + a.dy
        a.angle = a.angle + a.da
        wrap(a)
    end
end

local function checkCollisions()
    for bi = #bullets, 1, -1 do
        local b = bullets[bi]
        for ai = #asteroids, 1, -1 do
            local a = asteroids[ai]
            local dx = b.x - a.x
            local dy = b.y - a.y
            if dx*dx + dy*dy < a.radius * a.radius then
                explode(a.x, a.y, 8)
                table.remove(bullets, bi)
                if a.radius > 5 then
                    local nangle = math.random() * 2 * math.pi
                    local nspeed = 0.5 + math.random() * 1.5
                    table.insert(asteroids, {
                        x = a.x, y = a.y,
                        dx = math.cos(nangle) * nspeed,
                        dy = math.sin(nangle) * nspeed,
                        radius = a.radius - 4,
                        angle = 0,
                        da = (math.random() - 0.5) * 6,
                        verts = a.verts,
                    })
                    nangle = math.random() * 2 * math.pi
                    table.insert(asteroids, {
                        x = a.x, y = a.y,
                        dx = math.cos(nangle) * nspeed,
                        dy = math.sin(nangle) * nspeed,
                        radius = a.radius - 4,
                        angle = 0,
                        da = (math.random() - 0.5) * 6,
                        verts = a.verts,
                    })
                end
                table.remove(asteroids, ai)
                score = score + 10
                if #asteroids == 0 then
                    level = level + 1
                    startLevel()
                end
                break
            end
        end
    end

    if ship.alive and ship.invuln == 0 then
        for ai = #asteroids, 1, -1 do
            local a = asteroids[ai]
            local dx = ship.x - a.x
            local dy = ship.y - a.y
            if dx*dx + dy*dy < (a.radius + 5)^2 then
                explode(ship.x, ship.y, 16)
                ship.alive = false
                lives = lives - 1
                if lives <= 0 then
                    gameOver = true
                else
                    ship.x = W/2
                    ship.y = H/2
                    ship.dx = 0
                    ship.dy = 0
                    ship.alive = true
                    ship.invuln = 120
                end
                break
            end
        end
    end
end

local function updateParticles()
    for i = #particles, 1, -1 do
        local p = particles[i]
        p.x = p.x + p.dx
        p.y = p.y + p.dy
        p.life = p.life - 1
        if p.life <= 0 then table.remove(particles, i) end
    end
end

function playdate.update()
    if gameOver then
        gfx.setColor(gfx.kColorBlack)
        gfx.fillRect(0, 0, W, H)
        gfx.setColor(gfx.kColorWhite)
        gfx.drawText("GAME OVER", 140, 100)
        gfx.drawText("Score: " .. score, 150, 120)
        gfx.drawText("Press A to restart", 120, 140)
        gfx.display()
        local _, pushed, _ = playdate.getButtonState()
        if pushed & playdate.kButtonA ~= 0 then
            score = 0
            lives = 3
            level = 1
            gameOver = false
            particles = {}
            startLevel()
        end
        return
    end

    updateShip()
    updateBullets()
    updateAsteroids()
    checkCollisions()
    updateParticles()

    gfx.setColor(gfx.kColorBlack)
    gfx.fillRect(0, 0, W, H)
    gfx.setColor(gfx.kColorWhite)

    for _, a in ipairs(asteroids) do drawAsteroid(a) end

    for _, b in ipairs(bullets) do
        gfx.fillRect(b.x - 1, b.y - 1, 2, 2)
    end

    if ship.alive then
        if ship.invuln == 0 or (ship.invuln % 8 < 4) then
            drawShip(ship.x, ship.y, ship.angle, ship.thrusting)
        end
    end

    for _, p in ipairs(particles) do
        gfx.fillRect(p.x, p.y, 1, 1)
    end

    gfx.drawText("Score: " .. score, 5, 5)
    gfx.drawText("Lives: " .. lives, 5, 20)
    gfx.drawText("Level: " .. level, 340, 5)

    gfx.display()
end

startLevel()
