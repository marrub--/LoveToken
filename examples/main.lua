-- This file is placed under public domain.
local tokenizer = require("lua/tokenizer")
local ffi = require("ffi")

function love.load(arg)
	local initCfg = ffi.new("LT_Config") -- we don't need to set any options here
	local tk
	
	tokenizer:init(initCfg, "a.txt")
	
	while (tk.token ~= "TOK_EOF") do
		tk = tokenizer:getToken()
		
		if (tk.string ~= nil) then
			print(tk.string)
		else
			print(tk.token)
		end
	end
	
	tokenizer:quit()
end

function love.update(dt)
end

function love.draw(dt)
end