Print(10)
Http["lua"] = function(http)
	Print("Hello")
	for i,v in ipairs(http.params) do
		Print(i..' = '..v) 
	end
	for k,v in pairs(http.query) do
		Print(k..' : '..v) 
	end
	http:Write("God Save Me Hello")
end