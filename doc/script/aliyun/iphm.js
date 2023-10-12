

function arrayToHex(array)
{
    return array.reduce((accu, item) => {
        let a = item.toString(16)
        if (a.length < 2) a = "0" + a
        a=a.toUpperCase()
        return accu += a
    }, "")
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
const PKT_TYPE = {
    
    TYPE_ACK:       0,
    TYPE_ERROR:     1,
    
    TYPE_CAP:       2,         //channel capture data
    TYPE_CALI:      3,        //calibration
    TYPE_STAT:      4,
    TYPE_SETT:      5,
    TYPE_PARA:      6,
    TYPE_FILE:      7,
    TYPE_TIME:      8,
    
    TYPE_DFLT:      9,
    TYPE_REBOOT:    10,
    TYPE_FACTORY:   11,
    
    TYPE_HBEAT:     12,       //heartbeat
    TYPE_DATATO:    13
    
    TYPE_MAX:       14
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

function stat_data_parse(array,devid)
{
    var statObj = {};
    var pos=0;
    
    var method  = "thing.event.property.batch.post";
    var version = "1.0"
    
    var dataView = new DataView(array.buffer, 0);    
    
    var id = ""+devid;
    
    var batt = dataview.getInt8(pos, 1);
    pos += 1;
    
    var temp  = dataview.getInt16(pos, 1);
    pos += 1;
    
    var sig_s = dataview.getFloat32(pos, 1);
    pos += 4;
    
    var sig_q = dataview.getInt16(pos, 1);
    pos += 4;
    
    var time = dataview.getUint64(pos, 1);
    
    var params = {};
    
    properties["stat_data:battery"]         = batt;
    properties["stat_data:temperature"]     = temp;
    properties["stat_data:signal_strength"] = sig_s;
    properties["stat_data:signal_quality"]  = sig_q;
    
    params["properties"] = properties;
    
    statObj["method"]  = method;
    statObj["version"] = version;
    statObj["id"]      = id;
    statObj["params"]  = params;
    
    return statObj;
}

function ch_data_parse(array,devid)
{
    var i,j;
    var jsObj = {};
    var pos=0;
    var tmpObj = {};
    
    var method  = "thing.event.property.batch.post";
    var version = "1.0"
    
    var id = ""+devid;
    var dv = new DataView(array.buffer, 0);
    
    //parse ch_data_t data
    var ch = dv.getUint32(pos, 1);
    pos += 4;
    
    var time = dv.getUint64(pos, 1);
    pos += 8;
    
    var wavlen = dv.getUint32(pos, 1);
    pos += 4;
    
    var evlen = dv.getUint32(pos, 1);
    pos += 4;
    
    var params = {};
    var properties = {};
    
    var ev = [];
    for(i=0; i<EV_TYPE.EV_NUM; i++) {
        ev[i] = [];
    }
    
    var n_ev = 4;
    var ev_unit_len=16;
    var grps = evlen/(n_ev*ev_unit_len);
    
    //console.log("ch: "+ch+" time: "+time+" wavlen: "+wavlen+" evlen: "+evlen+" grps: "+grps+" pos: "+pos);
    
    //parse ev_data_t data
    for(i=0; i<grps; i++) {
        for(j=0; j<n_ev; j++) {
            var tp = dv.getUint32(pos, 1);
            var tm = dv.getUint64(pos+4, 1);
            var val = dv.getFloat32(pos+12, 1);
            
            //console.log("tp: "+tp+" val: "+val);
            ev[tp].push({"value": val, "time": tm});
            
            pos += ev_unit_len;
        }
    }
    
    properties["ev_data:channelid"] = [{"value": ch, "time": time}];
    
    for(i=0; i<EV_TYPE.EV_NUM; i++) {
        if(!isEmpty(ev[i])) {
            properties["ev_data:"+EV_STR[i]] = ev[i];
        }
    }
    
    params["properties"] = properties;
    
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
    
    var dv = new DataView(array.buffer, 0);
    
    var magic = dv.getUint32(pos, 1);
    
    console.log("magic: "+magic);
    
    pos += 4;
    if(magic!=PKT_MAGIC) {
        return dataObj;
    } 
    
    var devID = dv.getUint32(pos, 1);
    pos += 4;
    
    var type = dv.getUint8(pos, 1);
    pos += 1;
    if(type>=PKT_TYPE.TYPE_MAX) {
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
    
    
    if(type==PKT_TYPE.TYPE_CAP) {
        dataObj = ch_data_parse(subData,devID);
    }
    else if(type==PKT_TYPE.TYPE_STAT) {
        dataObj = stat_data_parse(subData,devID);
    }
    
    pktObj["magic"]   = magic;
    pktObj["devID"]   = devID;
    pktObj["type"]    = type;
    pktObj["flag"]    = flag;
    pktObj["askAck"]  = askAck;
    pktObj["datalen"] = dataLen;
    pktObj["data"]    = dataObj;
    
    return dataObj; //pktObj;
}


/////////////////////////////////////////////////////

function transformPayload(topic, rawData) {
    var jsonObj = {};
    
    if (topic.endsWith('/user/update')) {
        var uint8Array = new Uint8Array(rawData.length);
        for (var i = 0; i < rawData.length; i++) {
            uint8Array[i] = rawData[i] & 0xff;
        }
        var dataView = new DataView(uint8Array.buffer, 0);
    }
    
    
    return jsonObj;
}

function rawDataToProtocol(rawData)
{
    var obj={};
    var debug=false;

    var u8Array = new Uint8Array(rawData.length);
    
    for (var i = 0; i < rawData.length; i++) {
        u8Array[i] = rawData[i] & 0xff;
    }
    
    if(debug) {
        obj["data"] = arrayToHex(u8Array);
        obj["length"] = rawData.length;
        
        obj["id"]  = id; 
        obj["sig"] = signal;
        obj["cnt"] = cnt;
    }
    else {
        obj = pkt_data_parse(u8Array);
    }
    
    return obj;
}

function protocolToRawData(jsonObj) {
    var rawdata = [];
    return rawdata;
}