-- Dump RTP h.265 payload to raw h.265 file (*.265)
-- According to RFC7798 to dissector H265 payload of RTP to NALU, and write it
-- to from<sourceIp_sourcePort>to<dstIp_dstPort>.265 file. 
-- By now, we support Single NAL Unit Packets, Aggregation Packets (APs)
-- and Fragmentation Units (FUs) format RTP payload for H.265.
-- You can access this feature by menu "Tools"
-- Reference from Huang Qiangxiong (qiangxiong.huang@gmail.com)
-- Author: Yang Xing (hongch_911@126.com)
------------------------------------------------------------------------------------------------
do
    local version_str = string.match(_VERSION, "%d+[.]%d*")
    local version_num = version_str and tonumber(version_str) or 5.1
    local bit = (version_num >= 5.2) and require("bit32") or require("bit")

    function string.starts(String,Start)
        return string.sub(String,1,string.len(Start))==Start
     end
     
     function string.ends(String,End)
        return End=='' or string.sub(String,-string.len(End))==End
     end
    function get_temp_path()
        local tmp = nil
        if tmp == nil or tmp == '' then
            tmp = os.getenv('HOME')
            if tmp == nil or tmp == '' then
                tmp = os.getenv('USERPROFILE')
                if tmp == nil or tmp == '' then
                    tmp = persconffile_path('temp')
                else
                    tmp = tmp .. "/wireshark_temp"
                end
            else
                tmp = tmp .. "/wireshark_temp"
            end
        end
        return tmp
    end
    function get_ffmpeg_path()
        local tmp = nil
        if tmp == nil or tmp == '' then
            tmp = os.getenv('FFMPEG')
            if tmp == nil or tmp == '' then
                tmp = ""
            else
                if not string.ends(tmp, "/bin/") then
                    tmp = tmp .. "/bin/"
                end
            end
        end
        return tmp
    end

    -- for geting h265 data (the field's value is type of ByteArray)
    local f_h265 = Field.new("h265") 
    local f_rtp = Field.new("rtp") 
    local f_rtp_seq = Field.new("rtp.seq")
    local f_rtp_timestamp = Field.new("rtp.timestamp")

    local filter_string = nil

    -- menu action. When you click "Tools->Export H265 to file" will run this function
    local function export_h265_to_file()
        -- window for showing information
        local tw = TextWindow.new("Export H265 to File Info Win")
        local pgtw;
        
        -- add message to information window
        function twappend(str)
            tw:append(str)
            tw:append("\n")
        end
        
        local ffmpeg_path = get_ffmpeg_path()
        -- temp path
        local temp_path = get_temp_path()
        
        -- running first time for counting and finding sps+pps, second time for real saving
        local first_run = true 
        local writed_nalu_begin = false
        -- variable for storing rtp stream and dumping parameters
        local stream_infos = nil

        -- trigered by all h265 packats
        local list_filter = ''
        if filter_string == nil or filter_string == '' then
            list_filter = "h265"
        elseif string.find(filter_string,"h265")~=nil then
            list_filter = filter_string
        else
            list_filter = "h265 && "..filter_string
        end
        twappend("Listener filter: " .. list_filter .. "\n")
        local my_h265_tap = Listener.new("frame", list_filter)
        
        -- get rtp stream info by src and dst address
        function get_stream_info(pinfo)
            local key = "from_" .. tostring(pinfo.src) .. "_" .. tostring(pinfo.src_port) .. "_to_" .. tostring(pinfo.dst) .. "_" .. tostring(pinfo.dst_port)
            key = key:gsub(":", ".")
            local stream_info = stream_infos[key]
            if not stream_info then -- if not exists, create one
                stream_info = { }
                stream_info.filename = key.. ".265"
                -- stream_info.filepath = stream_info.filename
                -- stream_info.file,msg = io.open(stream_info.filename, "wb")
                if not Dir.exists(temp_path) then
                    Dir.make(temp_path)
                end
                stream_info.filepath = temp_path.."/"..stream_info.filename
                stream_info.file,msg = io.open(temp_path.."/"..stream_info.filename, "wb")
                if msg then
                    twappend("io.open "..stream_info.filepath..", error "..msg)
                end
                -- twappend("Output file path:" .. stream_info.filepath)
                stream_info.counter = 0 -- counting h265 total NALUs
                stream_info.counter2 = 0 -- for second time running
                stream_infos[key] = stream_info
                twappend("Ready to export H.265 data (RTP from " .. tostring(pinfo.src) .. ":" .. tostring(pinfo.src_port) 
                         .. " to " .. tostring(pinfo.dst) .. ":" .. tostring(pinfo.dst_port) .. " write to file:[" .. stream_info.filename .. "] ...")
            end
            return stream_info
        end
        
        -- write a NALU or part of NALU to file.
        local function write_to_file(stream_info, str_bytes, begin_with_nalu_hdr, end_of_nalu)
            if first_run then
                stream_info.counter = stream_info.counter + 1
                
                if begin_with_nalu_hdr then
                    -- save VPS SPS PPS
                    local nalu_type = bit.rshift(bit.band(str_bytes:byte(0,1), 0x7e),1)
                    if not stream_info.vps and nalu_type == 32 then
                        stream_info.vps = str_bytes
                    elseif not stream_info.sps and nalu_type == 33 then
                        stream_info.sps = str_bytes
                    elseif not stream_info.pps and nalu_type == 34 then
                        stream_info.pps = str_bytes
                    end
                end
                
            else -- second time running

                if not writed_nalu_begin then
                    if begin_with_nalu_hdr then
                        writed_nalu_begin = true
                    else
                        return
                    end
                end
                
                if stream_info.counter2 == 0 then
                    local nalu_type = bit.rshift(bit.band(str_bytes:byte(0,1), 0x7e),1)
                    if nalu_type ~= 32 then
                        -- write VPS SPS and PPS to file header first
                        if stream_info.vps then
                            stream_info.file:write("\x00\x00\x00\x01")
                            stream_info.file:write(stream_info.vps)
                        else
                            twappend("Not found VPS for [" .. stream_info.filename .. "], it might not be played!")
                        end
                        if stream_info.sps then
                            stream_info.file:write("\x00\x00\x00\x01")
                            stream_info.file:write(stream_info.sps)
                        else
                            twappend("Not found SPS for [" .. stream_info.filename .. "], it might not be played!")
                        end
                        if stream_info.pps then
                            stream_info.file:write("\x00\x00\x00\x01")
                            stream_info.file:write(stream_info.pps)
                        else
                            twappend("Not found PPS for [" .. stream_info.filename .. "], it might not be played!")
                        end
                    end
                end
            
                if begin_with_nalu_hdr then
                    -- *.265 raw file format seams that every nalu start with 0x00000001
                    stream_info.file:write("\x00\x00\x00\x01")
                end
                stream_info.file:write(str_bytes)
                stream_info.counter2 = stream_info.counter2 + 1

                -- update progress window's progress bar
                if stream_info.counter > 0 and stream_info.counter2 < stream_info.counter then
                    pgtw:update(stream_info.counter2 / stream_info.counter)
                end
            end
        end
        
        -- read RFC3984 about single nalu/ap/fu H265 payload format of rtp
        -- single NALU: one rtp payload contains only NALU
        local function process_single_nalu(stream_info, h265)
            write_to_file(stream_info, h265:tvb():raw(), true, true)
        end
        
        -- APs: one rtp payload contains more than one NALUs
        local function process_ap(stream_info, h265)
            local h265tvb = h265:tvb()
            local offset = 2
            repeat
                local size = h265tvb(offset,2):uint()
                write_to_file(stream_info, h265tvb:raw(offset+2, size), true, true)
                offset = offset + 2 + size
            until offset >= h265tvb:len()
        end
        
        -- FUs: one rtp payload contains only one part of a NALU (might be begin, middle and end part of a NALU)
        local function process_fu(stream_info, h265)
            local h265tvb = h265:tvb()
            local start_of_nalu = (h265tvb:range(2, 1):bitfield(0,1) ~= 0)
            local end_of_nalu =  (h265tvb:range(2, 1):bitfield(1,1) ~= 0)
            if start_of_nalu then
                -- start bit is set then save nalu header and body
                local nalu_hdr_0 = bit.bor(bit.band(h265:get_index(0), 0x81), bit.lshift(bit.band(h265:get_index(2),0x3F), 1))
                local nalu_hdr_1 = h265:get_index(1)
                write_to_file(stream_info, string.char(nalu_hdr_0, nalu_hdr_1) .. h265tvb:raw(3), start_of_nalu, end_of_nalu)
            else
                -- start bit not set, just write part of nalu body
                write_to_file(stream_info, h265tvb:raw(3), start_of_nalu, end_of_nalu)
            end
        end
        
        -- call this function if a packet contains h265 payload
        function my_h265_tap.packet(pinfo,tvb)
            if stream_infos == nil then
                -- not triggered by button event, so do nothing.
                return
            end
            local h265s = { f_h265() } -- using table because one packet may contains more than one RTP
            
            for i,h265_f in ipairs(h265s) do
                if h265_f.len < 5 then
                    return
                end
                local h265 = h265_f.range:bytes() 
                local hdr_type = h265_f.range(0,1):bitfield(1,6)
                local stream_info = get_stream_info(pinfo)
                
                if hdr_type > 0 and hdr_type < 48 then
                    -- Single NALU
                    process_single_nalu(stream_info, h265)
                elseif hdr_type == 48 then
                    -- APs
                    process_ap(stream_info, h265)
                elseif hdr_type == 49 then
                    -- FUs
                    process_fu(stream_info, h265)
                else
                    twappend("Error: No.=" .. tostring(pinfo.number) .. " unknown type=" .. hdr_type .. " ; we only know 1-47(Single NALU),48(APs),49(FUs)!")
                end
            end
        end
        
        -- close all open files
        local function close_all_files()
            twappend("")
            local index = 0;
            if stream_infos then
                local no_streams = true
                for id,stream in pairs(stream_infos) do
                    if stream and stream.file then
                        stream.file:flush()
                        stream.file:close()
                        stream.file = nil
                        index = index + 1
                        twappend(index .. ": [" .. stream.filename .. "] generated OK!")
                        local anony_fuc = function ()
                            twappend("ffplay -x 640 -y 640 -autoexit "..stream.filename)
                            --copy_to_clipboard("ffplay -x 640 -y 640 -autoexit "..stream.filepath)
                            os.execute(ffmpeg_path.."ffplay -x 640 -y 640 -autoexit "..stream.filepath)
                        end
                        tw:add_button("Play "..index, anony_fuc)
                        no_streams = false
                    end
                end
                
                if no_streams then
                    twappend("Not found any H.265 over RTP streams!")
                else
                    tw:add_button("Browser", function () browser_open_data_file(temp_path) end)
                end
            end
        end
        
        function my_h265_tap.reset()
            -- do nothing now
        end
        
        tw:set_atclose(function ()
            my_h265_tap:remove()
            if Dir.exists(temp_path) then
                Dir.remove_all(temp_path)
            end
        end)
        
        local function export_h265()
            pgtw = ProgDlg.new("Export H265 to File Process", "Dumping H265 data to file...")
            first_run = true
            stream_infos = {}
            -- first time it runs for counting h.265 packets and finding SPS and PPS
            retap_packets()
            first_run = false
            -- second time it runs for saving h265 data to target file.
            retap_packets()
            close_all_files()
            -- close progress window
            pgtw:close()
            stream_infos = nil
        end
        
        tw:add_button("Export All", function ()
            export_h265()
        end)

        tw:add_button("Set Filter", function ()
            tw:close()
            dialog_menu()
        end)
    end

    local function dialog_func(str)
        filter_string = str
        export_h265_to_file()
    end

    function dialog_menu()
        new_dialog("Filter Dialog",dialog_func,"Filter")
    end

    local function dialog_default()
        filter_string = get_filter()
        export_h265_to_file()
    end
    
    -- Find this feature in menu "Tools"
    register_menu("Video/Export H265", dialog_default, MENU_TOOLS_UNSORTED)
end
