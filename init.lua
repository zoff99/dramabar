if srv~=nil then
  srv:close()
end
wifi.setmode(wifi.STATION)
wifi.sta.config("metalab","")
wifi.sta.setip({ip="10.20.30.22",netmask="255.255.0.0",gateway="10.20.30.1"});
srv=net.createServer(net.TCP)

local httpRequest={};
httpRequest["/"]="dramabarsite.html";
httpRequest["/home"]="dramabarsite.html";
httpRequest["/dramabarsite.html"]="dramabarsite.html";
httpRequest["/credits"]="credits.html";
httpRequest["/defaults"]="dramabarsite.html";

local getContentType={};
getContentType["/"]="dramabarsite.html";
getContentType["/home"]="dramabarsite.html";
getContentType["/dramabarsite.html"]="dramabarsite.html";
getContentType["/credits"]="credits.html";
getContentType["/defaults"]="dramabarsite.hmtl";

local pos = 0;
local chunksize = 1024;

function filesize (filename)
    local buf = file.list();
    for name,size in pairs(buf) do
        if (name == filename) then
            return size;
        end
    end
end

srv:listen(80,function(conn)
    conn:on("receive", function(conn,request)
        --print("Request:\n" ..request.. "\nRequest end\n");
        if (string.match(request, 'POST')) then
            s, e = string.find(request, "cH");
            data = string.sub(request, s);
            uart.write(0, data, "\0");
        elseif (string.match(request, "GET /favicon.ico")) then
            conn:send("HTTP/1.1 404 Not found\r\n\r\n");
            conn:close();
            collectgarbage();
        end
        local _, _, method, path, vars = string.find(request, "([A-Z]+) (.+)?(.+) HTTP");
        if(method == nil)then
            _, _, method, path = string.find(request, "([A-Z]+) (.+) HTTP");
        end
        local _GET = {}
        if (vars ~= nil)then
            for k, v in string.gmatch(vars, "(%w+)=(%w+)&*") do
                _GET[k] = v
            end
        end

        header = 'HTTP/1.1 200 OK\r\n';
        if getContentType[path] then
            requestFile = httpRequest[path];
            pos = 0;
            conn:send("HTTP/1.1 200 OK\r\nContent-Type: " ..getContentType[path].."\r\n\r\n");
        else
            --print("File not found (invalid path?): " ..path);
            conn:send("HTTP/1.1 404 Not found\r\n\r\n");
            conn:close();
            collectgarbage();
        end

        if (filesize(requestFile) > chunksize) then
            conn:send(header);
        else
            file.open(requestFile, "r");
            conn:send(header..file.read())
            file.close();
        end
    end)
    conn:on("sent", function(conn)
        if requestFile then
            if file.open(requestFile, r) then
                --print("File: " ..requestFile);
                file.seek("set", pos*chunksize);
                partialData = file.read();
                file.close();
                if (partialData ~= 0) then
                    pos = pos + 1;
                    conn:send(partialData);
                    if (#partialData == chunksize) then
                        return;
                    else
                        pos = 0;
                    end
                end
            else
            --print("Error with opening file " ..requestFile);
            end
        conn:close();
        collectgarbage();
        end
    end)
end)
