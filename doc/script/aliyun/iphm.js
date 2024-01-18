
function toFloatSpecial(x, n)
{
    var y = x.toFixed(2);
    var fnum = parseFloat(y);
	var fstr = fnum.toString();
	var dloc = fstr.indexOf('.');
	if (dloc < 0) {
        fnum += 1/Math.pow(10,n);
	}
	return fnum;
}

function arrayToHex(array)
{
    return array.reduce((accu, item) => {
        let a = item.toString(16)
        if (a.length < 2) a = "0" + a
        a=a.toUpperCase()
        return accu += a
    }, "")
}

function destName(dest)
{
    if(dst==="lab") {
        //get device name
        return "lab_"+deviceName().substring(3);
    }
    else if(dst==="pc"){
        //get client name
        return "PC_"+deviceName().substring(4);
    }
    
    return "";
}



DataView.prototype.getUint64 = function(byteOffset, littleEndian)
{
    const left =  this.getUint32(byteOffset, littleEndian);
    const right = this.getUint32(byteOffset+4, littleEndian);

    const combined = littleEndian? left + Math.pow(2,32)*right : Math.pow(2,32)*left + right;

    //if (!Number.isSafeInteger(combined)) {
    //    console.warn(combined, 'exceeds MAX_SAFE_INTEGER. Precision may be lost');
    //}

    return combined;
}


var do_constantize = (obj) => {
    Object.freeze(obj);
    Object.keys(obj).forEach( (key, i) => {
        if ( typeof obj[key] === 'object' ) {
            constantize( obj[key] );
        }
    });
}


function isEmpty(obj)
{
    return Object.keys(obj).length === 0;
}

////////////////////////////////////////////////////////////
const PKT_MAGIC=0xDEADBEEF;

const DEV_TYPE = {
    DTYPE_NA:       0,    
    DTYPE_XAEDAU100:       1,        //设备类型    
    DTYPE_MAX:       2
};

const PKT_TYPE = {
    TYPE_ACK:       0,
    
    TYPE_CAP:       1,        //channel capture data
    TYPE_CALI:      2,        //calibration
    TYPE_STAT:      3,
    TYPE_DAC:       4,
    TYPE_MODE:      5,
    TYPE_SETT:      6,
    TYPE_PARA:      7,
    TYPE_FILE:      8,
    TYPE_TIME:      9,
    
    TYPE_DFLT:      10,
    TYPE_REBOOT:    11,
    TYPE_FACTORY:   12,
    
    TYPE_HBEAT:     13,       //heartbeat
    TYPE_DATO:      14,
    
    TYPE_MAX:       15
};
const EV_TYPE = {
    EV_RMS:0,
    EV_AMP:1,
    EV_ASL:2,
    EV_ENE:3,
    EV_AVE:4,         //average
    EV_MIN:5,
    EV_MAX:6,
    
    EV_NUM:7
};

const EV_STR=[
    "rms",
    "amp",
    "asl",
    "ene",
    "ave",
    "min",
    "max",
];

function stat_data_parse(array,devid, msgType, devType)
{
    var statObj = {};
    var pos=0;
    
    var method  = "thing.event.property.batch.post";
    var version = "1.0"
    
    var dv = new DataView(array.buffer, 0);    
    
    var id = ""+devid;
    
    var rssi = dv.getInt32(pos, 1);
    pos += 4;
    
    var ber = dv.getInt32(pos, 1);
    pos += 4;
    
    
    var vbat = dv.getFloat32(pos, 1);
    pos += 4;
    
    var temp  = dv.getFloat32(pos, 1);
    pos += 4;
    
    var time = dv.getUint64(pos, 1);
    
    var params = {};
    var properties = {};
    
    properties["stat_data:rssi"]    = [{"value": rssi, "time": time}];
    properties["stat_data:ber"]     = [{"value": ber,  "time": time}];
    properties["stat_data:vbat"]    = [{"value": toFloatSpecial(vbat,2), "time": time}];
    properties["stat_data:temp"]    = [{"value": toFloatSpecial(temp,2), "time": time}];
	properties["dev_id"] = [{"value": devid, "time": time}];
	properties["msg_type"] = [{"value": msgType, "time": time}];
	properties["dev_type"] = [{"value": devType, "time": time}];
    
    params["properties"] = properties;
    
    statObj["method"]  = method;
    statObj["version"] = version;
    statObj["id"]      = id;
    statObj["params"]  = params;
    
    return statObj;
}

function ch_data_parse(array,devid, msgType, devType)
{
    var i,j;
    var jsObj = {};
    var pos=0;
    var tmpObj = {};
    
    var method  = "thing.event.property.batch.post";
    var method2 = "user.update";
    var version = "1.0"
    
    var id = ""+devid;
    var dv = new DataView(array.buffer, 0);
    
    //parse ch_data_t data
    var ch = dv.getUint32(pos, 1);
    pos += 4;
    
    var time = dv.getUint64(pos, 1);
    pos += 8;
    
    var smpFreq = dv.getUint32(pos, 1);
    pos += 4;
    
    var wavlen = dv.getUint32(pos, 1);
    pos += 4;
    
    var evlen = dv.getUint32(pos, 1);
    pos += 4;
    
    var params = {};
    var properties = {};
       
    if(evlen>0) {
        var ev = [];
        for(i=0; i<EV_TYPE.EV_NUM; i++) {
            ev[i] = [];
        }
        
        //parse ev_data_t
        pos += wavlen;
        var grps = dv.getUint32(pos, 1);
        pos += 4;
        
        //console.log("ch: "+ch+" time: "+time+" wavlen: "+wavlen+" evlen: "+evlen+" grps: "+grps+" pos: "+pos);
        
        //parse ev_data_t data
        for(i=0; i<grps; i++) {
            var tm = dv.getUint64(pos, 1);
            var cnt = dv.getUint32(pos+8, 1);
            pos += 12;
            
            for(j=0; j<cnt; j++) {
                var tp = dv.getUint32(pos, 1);
                var val = dv.getFloat32(pos+4, 1);
                
                //console.log("tp: "+tp+" val: "+val);
                ev[tp].push({"value": toFloatSpecial(val, 2), "time": tm});
                
                pos += 8;
            }
        }
        
        properties["ev_data:channelid"] = [{"value": ch, "time": time}];
		properties["dev_id"] = [{"value": devid, "time": time}];
		properties["msg_type"] = [{"value": msgType, "time": time}];
		properties["dev_type"] = [{"value": devType, "time": time}];
		properties["smp_freq"] = [{"value": smpFreq, "time": time}];
        
        for(i=0; i<EV_TYPE.EV_NUM; i++) {
            if(!isEmpty(ev[i])) {
                properties["ev_data:"+EV_STR[i]] = ev[i];
            }
        }
        
        params["properties"] = properties;
    }
    
    jsObj["method"]  = method;
    jsObj["version"] = version;
    jsObj["id"]      = id;
    jsObj["params"]  = params;
    
    return jsObj;   
}
function pkt_data_parse(array)
{
    var pos=0;
    var pktObj={};
    var dataObj={};
	var devType = DEV_TYPE.DTYPE_XAEDAU100
    
    var dv = new DataView(array.buffer, 0);
    
    var magic = dv.getUint32(pos, 1);
    
    pos += 4;
    if(magic!=PKT_MAGIC) {
        console.log("wrong magic\n");
        return dataObj;
    } 
    
    var devID = dv.getUint32(pos, 1);
    pos += 4;
    
    var msgType = dv.getUint8(pos, 1);
    pos += 1;
    if(msgType>=PKT_TYPE.TYPE_MAX) {
        console.log("wrong msgType\n");
        return dataObj;
    }
    
    var flag = dv.getUint8(pos, 1);
    pos += 1;
    
    var askAck = dv.getUint8(pos, 1);
    pos += 1;
    
    var dataLen = dv.getUint16(pos, 1);
    pos += 2;
    
    //var subData = array.subarray(pos);
    var subData = array.slice(pos);
	
    if(msgType==PKT_TYPE.TYPE_CAP) {
        dataObj = ch_data_parse(subData,devID, msgType, devType);
    }
    else if(msgType==PKT_TYPE.TYPE_STAT) {
		dataObj = stat_data_parse(subData,devID, msgType, devType);
    }
    
    pktObj["magic"]   = magic;
    pktObj["devID"]   = devID;
    pktObj["type"]    = msgType;
    pktObj["flag"]    = flag;
    pktObj["askAck"]  = askAck;
    pktObj["datalen"] = dataLen;
    //pktObj["data"]    = dataObj;
    
    return dataObj;
    //return pktObj;
}


/////////////////////////////////////////////////////
function rawToArray(raw)
{
    var u8Array = new Uint8Array(raw.length);

    for (var i = 0; i < raw.length; i++) {
        u8Array[i] = raw[i] & 0xff;
    }
    
    return u8Array;
}



function transformPayload(topic, rawData)
{
    var jsonObj = {};
    
    var u8Array = rawToArray(rawData);
    if (topic.includes("user/set") || topic.includes("user/wav")) {
        jsonObj = pkt_data_parse(u8Array);
    }
    
    return jsonObj;
}

function rawDataToProtocol(rawData)
{
    var jsonObj={};

    var u8Array = rawToArray(rawData);
    jsonObj = pkt_data_parse(u8Array);
 
    return jsonObj;
}

function protocolToRawData(jsonObj)
{
    var rawdata = [];
    return rawdata;
}