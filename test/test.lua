--Print(10)
r = Redis.New("127.0.0.1", 6379)
r:Call("set", "god", "zhuchuanjing")
Print(r:Call("get", "aaa"))
Print(r:Call("get", "god"))
r:Delete()

Http["get"] = function(http)
	--Print("Hello")
	local content = ""
	for k,v in pairs(http.query) do
		content = content..k..' : '..v 
	end
	http:Write(http.url..content)
end