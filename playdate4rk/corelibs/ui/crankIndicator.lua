import 'CoreLibs/assets/crank/crank-frames-1x-table-52-38.png'
import 'CoreLibs/assets/crank/crank-frames-2x-table-26-19.png'
import 'CoreLibs/assets/crank/crank-frames-4x-table-13-10.png'
import 'CoreLibs/assets/crank/crank-frames-8x-table-6-5.png'
import 'CoreLibs/assets/crank/crank-notice-bubble-1x.png'
import 'CoreLibs/assets/crank/crank-notice-bubble-2x.png'
import 'CoreLibs/assets/crank/crank-notice-bubble-4x.png'
import 'CoreLibs/assets/crank/crank-notice-bubble-8x.png'
import 'CoreLibs/assets/crank/crank-notice-bubble.png'
import 'CoreLibs/assets/crank/crank-notice-text-1x.png'
import 'CoreLibs/assets/crank/crank-notice-text-2x.png'

local gfx <const> = playdate.graphics
playdate.ui = playdate.ui or {}
playdate.ui.crankIndicator = { clockwise = true }

local crankIndicatorY = 210
local textOffset = 76
local currentScale = nil
local currentFrame = 1
local frameCount = 0
local textFrameCount = 14
local lastTime = 0
local bubbleImage = nil
local bubbleX = nil
local bubbleY = nil
local bubbleWidth = nil
local bubbleHeight = nil
local bubbleFlip = playdate.graphics.kImageUnflipped
local crankFrames = nil
local textFrameImage = nil
local lastScale = nil

local function loadImagesForScale(scale)

	lastTime = 0
	currentFrame = 1
	currentScale = playdate.display.getScale()
	
	crankIndicatorY = 210 // scale
	bubbleImage = gfx.image.new('CoreLibs/assets/crank/crank-notice-bubble-'..scale..'x')
	bubbleWidth, bubbleHeight = bubbleImage:getSize()
	
	if playdate.getFlipped() then
		bubbleX = 0
		bubbleY = playdate.display.getHeight() - crankIndicatorY - bubbleHeight // 2
		bubbleFlip = playdate.graphics.kImageFlippedXY
		textOffset = 100 // scale
	else
		bubbleX = playdate.display.getWidth() - bubbleWidth
		bubbleY = crankIndicatorY - bubbleHeight // 2
		bubbleFlip = playdate.graphics.kImageUnflipped
		textOffset = 76 // scale
	end
	
	crankFrames = gfx.imagetable.new('CoreLibs/assets/crank/crank-frames-'..scale..'x')
	frameCount = #crankFrames * 3
	
	if scale <= 2 then
		textFrameImage = gfx.image.new('CoreLibs/assets/crank/crank-notice-text-'..scale..'x')
		textFrameCount = 14
		frameCount += textFrameCount
	else
		textFrameImage = nil
		textFrameCount = 0
	end
end


function playdate.ui.crankIndicator:draw(xOffset, yOffset)
	
	if xOffset == nil then xOffset = 0 end
	if yOffset == nil then yOffset = 0 end
	
	local scale = playdate.display.getScale()
	if scale ~= 2 and scale ~= 4 and scale ~= 8 then scale = 1 end
	if  currentScale ~= scale then
		loadImagesForScale(scale)
	end
	
	local currentTime = playdate.getCurrentTimeMilliseconds()
	local delta = currentTime - lastTime

	-- reset to start frame if :draw() hasn't been called in more than a second
	if delta > 1000 then
		currentFrame = 1
		delta = 0
		lastTime = currentTime
	end

	-- calculate how many frames the animation should jump ahead (how long has it been since this was last called?)
	while delta >= 49 do
		lastTime += 50
		delta -= 50
		currentFrame += 1
		if currentFrame > frameCount then
			currentFrame = 1
		end
	end	
	
	gfx.pushContext()
	
	bubbleImage:drawIgnoringOffset(bubbleX + xOffset, bubbleY + yOffset, bubbleFlip)
	
	if scale > 2 or currentFrame > textFrameCount then
		-- draw crank frames
		local frame = nil
		if self.clockwise then
			frame = crankFrames[((currentFrame-textFrameCount-1) % #crankFrames) + 1]
		else
			frame = crankFrames[((#crankFrames - (currentFrame-textFrameCount-1)) % #crankFrames) + 1]
		end
		local frameWidth, frameHeight = frame:getSize()
		if scale == 2 or scale == 4 then 
			yOffset += 1
		end
		frame:drawIgnoringOffset(bubbleX + xOffset + (textOffset - frameWidth) // 2, bubbleY + yOffset + (bubbleHeight - frameHeight) // 2)

	else
		-- draw text
		if textFrameImage ~= nil then
			local textWidth, textHeight = textFrameImage:getSize()
			if scale == 2 then 
				xOffset -= 1
			end
			textFrameImage:drawIgnoringOffset(bubbleX + xOffset + (textOffset - textWidth) // 2, bubbleY + yOffset + (bubbleHeight - textHeight) // 2)
		end
	end
	
	gfx.popContext()	
end


function playdate.ui.crankIndicator:getBounds(xOffset, yOffset)
	
	if playdate.display.getScale() ~= lastScale then
		lastScale = playdate.display.getScale()
		loadImagesForScale(lastScale)
	end
	
	if xOffset == nil then xOffset = 0 end
	if yOffset == nil then yOffset = 0 end
	
	return bubbleX + xOffset, bubbleY + yOffset, bubbleWidth, bubbleHeight
end


function playdate.ui.crankIndicator:resetAnimation()
	lastTime = playdate.getCurrentTimeMilliseconds()
	currentFrame = 1
end




-- deprecated methods
function playdate.ui.crankIndicator:update(xOffset)
	playdate.ui.crankIndicator:draw(xOffset)
end
function playdate.ui.crankIndicator:start()
end
