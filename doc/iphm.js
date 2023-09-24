
function arrToHex(arr)
{
    return arr.reduce((accu, item) => {
        let a = item.toString(16)
        if (a.length < 2) a = "0" + a
        a=a.toUpperCase()
        return accu += a
    }, "")
}
DataView.prototype.getUint64 = function(byteOffset, littleEndian) {

  const left =  this.getUint32(byteOffset, littleEndian);
  const right = this.getUint32(byteOffset+4, littleEndian);

  const combined = littleEndian? left + Math.pow(2,32)*right : Math.pow(2,32)*left + right;

  if (!Number.isSafeInteger(combined))
    console.warn(combined, 'exceeds MAX_SAFE_INTEGER. Precision may be lost');

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
  return Object.keys(obj).length === 0
}

////////////////////////////////////////////////////////////
const var PKT_MAGIC=0xDEADBEEF;
const PKT_TYPE = {
    TYPE_CAP:   0,       //channel capture data
    TYPE_CALI:  1,        //calibration
    TYPE_ACK:   2,
    TYPE_STA:   3,
    TYPE_SETT:  4,
    TYPE_PARA:  5,
    TYPE_FILE:  6,
    TYPE_TIME:  7,
    TYPE_HBEAT: 8,
    TYPE_ERROR: 9,
    
    TYPE_MAX:   10
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

const EV_STR[EV_TYPE.EV_NUM]={
    "rms",
    "amp",
    "asl",
    "ene",
    "ave",
    "min",
    "max",
};

function stat_data_parse(dataArray)
{
    var jsObj = {};
    var pos=0;
    
    var method  = "thing.event.property.batch.post";
    var version = "1.0"
    
    var dataView = new DataView(dataArray.buffer, 0);    
    
    var id = "123978987393";
    
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
    
    jsObj["method"]  = method;
    jsObj["version"] = version;
    jsObj["id"]      = id;
    jsObj["params"]  = params;
    
    return jsObj;   
    
    
    
}
function cap_data_parse(dataArray)
{
    var jsObj = {};
    var pos=0;
    var tmpObj = {};
    
    var method  = "thing.event.property.batch.post";
    var version = "1.0"
    
    var dataView = new DataView(dataArray.buffer, 0);
    var id = "3439783243546";
    
    var chid = dataview.getUint16(pos, 1);
    pos += 2;
    
    var time = dataview.getUint64(pos, 1);
    pos += 8;
    
    var dlen = dataview.getUint32(pos, 1);
    pos += 4;
    
    var evlen = dataview.getUint32(pos, 1);
    pos += 4;
    
    var params = {};
    var properties = {};
    
    var evObj = [];
    for(var i=0; i<EV_TYPE.EV_NUM; i++>) {
        evObj[i] = {};
    }
    
    var tlen=0;
    var ev_cnt = evlen/13;
    
    var ev = new DataView(dataArray.buffer, pos);
    for(i=0; i<ev_cnt; i++) {
        var tp = ev.getUint64(tlen, 1);
        var tm = ev.getUint64(tlen+1, 1);
             
        evObj[tp] = {"value": ev.getFloat32(tlen+9,   1), "time": tm};
        tlen += 13;
    }
    
    var chid = [];
    
    chid[0] = {"value": channelid, "time": time};
    ssid[0] = {"value": sensorid, "time": time};
    sig[0]  = {"value": signal, "time": time};
    
    properties["ev_data:channelid"] = chid;
    properties["ev_data:sensorid"]  = ssid;
    properties["ev_data:signal"]    = sig;
    
    for(var i=0; i<EV_TYPE.EV_NUM; i++>) {
        if(!isEmptyevObj[i]) {
            properties["ev_data:"+EV_STR[i]] = evObj[i];
        }
    }
    
    params["properties"] = properties;
    
    jsObj["method"]  = method;
    jsObj["version"] = version;
    jsObj["id"]      = id;
    jsObj["params"]  = params;
    
    return jsObj;   
}
function pkt_data_parse(dataArray)
{
    var pos=0;
    var pktObj={};
    var dataObj={};
    
    var dataView = new DataView(dataArray.buffer, 0);
    
    var magic = dataview.getUint32(pos, 1);
    pos += 4;
    if(magic!=PKT_MAGIC) {
        return null;
    } 
        
    var type = dataview.getUint8(pos, 1);
    pos += 1;
    if(type>=PKT_TYPE.TYPE_MAX) {
        return null;
    }
    
    var flag = dataview.getUint8(pos, 1);
    pos += 1;
    
    var askAck = dataview.getUint8(pos, 1);
    pos += 1;
    
    var dataLen = dataview.getUint16(pos, 1);
    pos += 2;
    
    var xdata = u8Array.subarray(pos);
    if(type==PKT_TYPE.TYPE_CAP) {
        dataObj = cap_data_parse(xdata);
    }
    else if(type==PKT_TYPE.TYPE_STAT) {
        dataObj = stat_data_parse(xdata);
    }
    
    pktObj["magic"]   = magic;
    pktObj["type"]    = type;
    pktObj["flag"]    = id;
    pktObj["askAck"]  = askAck;
    pktObj["datalen"] = datalen;
    pktObj["data"]    = dataObj;
    
    return dataObj; //pktObj;
}


/////////////////////////////////////////////////////

function transformPayload(topic, rawData) {
    var jsonObj = {};
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
        tmpObj["data"] = arrToHex(u8Array);
        tmpObj["length"] = rawData.length;
        
        tmpObj["id"]  = id; 
        tmpObj["sig"] = signal;
        tmpObj["cnt"] = cnt;
        return tmpObj;
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