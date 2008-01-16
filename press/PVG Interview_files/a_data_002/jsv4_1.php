var f=null;
var tbl=null;
function c2init(){
f=document.forms.cbox;
rsz(0);
var x=getcookie("key_"+s_id);
if(x!=null&&!s_uo){
f.key.value=x;
}
if(s_uo&&(f.nme.value==""||f.key.value=="")){
f.pst.value=t0;
f.pst.disabled=true;
f.sub.disabled=true;
}
try{
parent.cboxmain.cinit();
}
catch(e){
}
}
function rsz(v){
var w=self.innerWidth;
if(isNaN(w)||w<=0){
w=document.body.clientWidth;
}
if(w>0){
eval(s_rz);
}else{
if(v<500){
window.setTimeout("rsz("+(v+1)+")",100);
}
}
}
function cb_checkform(){
if(f.nme.value==t1){
alert(t2);
f.nme.focus();
return false;
}
if(f.pst.value==t3){
alert(t4);
f.pst.focus();
return false;
}
if(f.eml){
if(f.eml.value!=""&&f.eml.value!=t5&&(f.eml.value.lastIndexOf(".")<=0&&f.eml.value.lastIndexOf("@")<=0)){
alert(t6);
f.eml.focus();
return false;
}
}
return true;
}
function frmfocus(x,y){
(x.value==y)?x.value="":void (0);
}
function frmblur(x,y){
(x.value=="")?x.value=y:void (0);
}
function pop(_8,w,h,s){
nw=window.open("./?"+s_rq+"&sec="+_8,"cb"+s_id+_8.substring(0,3),"width="+w+", height="+h+", toolbar=no, scrollbars="+s+", status=no, resizable=yes");
try{
x=screen.width;
y=screen.height;
nw.moveTo((x/2)-(w/2)-100,(y/2)-(y/4));
nw.focus();
}
catch(e){
}
}
function aonliners(x){
if(x&&document.getElementById("onliners")){
document.getElementById("onliners").innerHTML=x+"&nbsp;"+((x==1)?t7:t8);
}
}
function crtn(e){
if(window.event){
k=window.event.keyCode;
}else{
if(e){
k=e.which;
}
}
if(k==13){
if(do_post()){
f.submit();
}
return false;
}else{
return true;
}
}
function p_open(){
if(f.nme.value==""||f.nme.value==t1){
f.nme.focus();
alert(t2);
}else{
pop("profile&n="+esc(f.nme.value)+"&k="+f.key.value,320,220,0);
}
}
var cexp=new Date((new Date()).getTime()+86400000*7).toGMTString();
var ptmp="";
function do_post(){
if(!cb_checkform()){
return false;
}
if(tbl==null){
return true;
}
set_status("Posting...");
ptmp=f.pst.value;
f.pst.value="";
f.pst.focus();
if(!http("POST","index.php?"+s_rq+"&sec=submit","nme="+esc(f.nme.value)+"&eml="+((f.eml)?esc(f.eml.value):"")+"&key="+f.key.value+"&pst="+esc(ptmp)+"&aj=x&lp="+lp,"post_proc",false,ptmp)){
f.pst.value=ptmp;
return true;
}
return false;
}
function esc(s){
if(encodeURIComponent==null){
function encodeURIComponent(s){
function eC(c){
function eIB(b){
return (eB(b,128));
}
function eB(b,_13){
b+=_13;
return ("%"+b.toString(16).toUpperCase());
}
function nB(c){
if(c<=127){
return (1);
}
if(c<=2047){
return (2);
}
return (3);
}
var nb=nB(c);
var so="";
for(var i=1;i<nb;i++){
so=eIB(c&63)+so;
c=c>>>6;
}
var a=new Array(0,192,224);
so=eB(c,a[nb-1])+so;
return (so);
}
var so="";
for(var i=0;i<s.length;i++){
var n=s.charCodeAt(i);
if(n<128){
var c=String.fromCharCode(n);
if("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.!~*'()".indexOf(c)!=-1){
so+=c;
}else{
so+=eC(n);
}
}else{
so+=eC(n);
}
}
return (so);
}
}
return encodeURIComponent(s);
}
var rlk=false;
function do_refresh(){
if(tbl==null){
return true;
}
set_status(t9);
if(!rlk){
ar_reset();
rlk=true;
window.setTimeout("rlk = false",10*1000);
if(!http("GET","./?"+s_rq+"&sec=ar&p="+lp+"&c="+((new Date()).getTime()),null,"aj_proc",false,true)){
return true;
}
}else{
window.setTimeout("set_status(t10)",500);
}
return false;
}
var wn=true;
function getlvl(){
if(s_uo){
return (f.key.value)?1:0;
}
return f.key.value.substring(f.key.value.length-1,f.key.value.length)*1;
}
function nme_warn(){
if(!wn&&f.nme.value!=""&&getlvl()<2){
wn=true;
alert(t11);
}
}
var tot=0;
function post_proc(x,p){
dat=x.split("\n");
hdr=dat[0].split("\t");
if((hdr[0]||!x)&&(f.pst.value==t3||f.pst.value=="")){
f.pst.value=p;
set_status(t12);
}else{
tot++;
}
f.pst.focus();
if(hdr[3]){
f.key.value=hdr[3];
}
if(hdr[0]){
alert(hdr[0]);
}
if(s_ap&&tot==1&&document.cookie==""){
alert(t13);
}
aj_proc(x);
}
var lp=0,op=0,ard=0,mylp=0;
function aj_proc(x,ss){
if(!x){
if(ss){
set_status(t10);
}
return false;
}
dat=x.split("\n");
hdr=dat[0].split("\t");
for(var i=dat.length-1;i>0;i--){
t=dat[i].split("\t");
if(t[0]>0){
lp=t[0];
}
add_post(t);
}
if(hdr[1]){
aonliners(hdr[1]);
}
if(hdr[4]){
ard=hdr[4]*1;
}else{
ard=Math.min(120,ard+5);
}
if(hdr[2]){
upd_tms(hdr[2]);
}
if(hdr[5]){
mylp=hdr[5];
}
if(dat.length-1>0){
if(s_sn){
snd=document.csnd;
try{
snd.DoPlay();
}
catch(e){
try{
snd.Play();
}
catch(f){
}
}
}
delban();
}else{
if(ss){
set_status(t10);
}
}
ar_reset();
}
var art=null;
var artc=null;
function ar_reset(){
if(art!=null){
window.clearTimeout(art);
}
if(s_ar&&ard>0){
art=window.setTimeout("ar_check(true)",ard*1000);
}
}
function ar_check(s){
if(!http("GET","./?"+s_rq+"&sec=ar&p="+lp+"&c="+((new Date()).getTime()),null,"aj_proc",s)){
if(ard<10){
ard=10;
}
i=new Image();
i.src="archeck_o.php?"+s_rq+"&rnd="+Math.random();
if(artc==null){
ar_poll();
}
}
ar_reset();
return true;
}
function ar_poll(){
var dc=document.cookie;
var lpc=getcookie("a"+s_id);
if(lpc>lp){
parent.cboxmain.document.location="index.php?"+s_rq+"&sec=main";
return true;
}
artc=window.setTimeout("ar_poll()",1000);
}
function getcookie(x){
var dc=document.cookie;
var i=dc.indexOf(x+"=",0);
if(i>-1){
var n=dc.indexOf(";",i);
n=(n==-1)?dc.length:n;
return dc.substring(i+x.length+1,n);
}
return null;
}
function http(m,u,d,f,s,c){
var r=null;
var t=0,p=0,q=0;
if(window.XMLHttpRequest){
r=new XMLHttpRequest();
}else{
if(window.ActiveXObject){
try{
r=new ActiveXObject("Msxml2.XMLHTTP");
}
catch(e){
try{
r=new ActiveXObject("Microsoft.XMLHTTP");
}
catch(e){
}
}
}
}
if(!r){
return false;
}
r.onreadystatechange=function(){
if(r.readyState==4){
window.clearTimeout(w);
if(r.status==200){
t=r.responseText;
q=t.substring(0,1);
p=t.substring(1);
if(q!="1"&&q!="0"){
if(!s){
alert(t14+"bad return");
}
p="";
}else{
if(q=="0"){
if(!s){
alert(p);
}
p="";
}
}
eval(f+"(p, c)");
}else{
if(!s){
alert(t14+r.status);
eval(f+"('', c)");
}
}
}
};
this.hfail=function(){
r.onreadystatechange=function(){
};
r.abort();
if(!s){
alert(t14+"timeout");
eval(f+"('', c)");
}
};
var w=window.setTimeout(this.hfail,20000);
r.open(m,u,true);
r.setRequestHeader("Connection","close");
if(d){
r.setRequestHeader("Content-Type","application/x-www-form-urlencoded");
}
r.send(d);
return true;
}
function set_status(s){
var p=new Array();
p[0]=0;
p[1]=0;
p[2]="";
p[3]="";
p[4]=0;
p[5]="";
p[6]=s;
if(s){
add_post(p);
}
}
var op;
function add_post(t){
if(t[0]>0&&parent.cboxmain.document.getElementById(t[0])){
return true;
}
var cnt=tbl.rows.length;
var cn=0;
var fs=false,ls=false;
if(cnt>0){
if(tbl.rows[0].id==0){
tbl.deleteRow(0);
cnt--;
}
if(cnt>1&&tbl.rows[cnt-1].id==0){
tbl.deleteRow(cnt-1);
cnt--;
}
if(cnt>1&&tbl.rows[1].id==0){
tbl.deleteRow(1);
cnt--;
}
if(cnt>1&&tbl.rows[cnt-2].id==0){
tbl.deleteRow(cnt-2);
cnt--;
}
}
if(cnt>0){
if(((cnt==1&&!s_sd)||cnt>1)&&tbl.rows[0].id==-1){
fs=true;
}
if(((cnt==1&&s_sd)||cnt>1)&&tbl.rows[cnt-1].id==-1){
ls=true;
}
var lc=tbl.rows[cnt-1].cells[0];
var fc=tbl.rows[0].cells[0];
var lm=(cnt-(ls?1:0)-(fs?1:0)>0)?tbl.rows[cnt-1-(ls?1:0)].cells[0]:null;
var fm=(cnt-(ls?1:0)-(fs?1:0)>0)?tbl.rows[fs?1:0].cells[0]:null;
if(!s_sd){
y=tbl.insertRow(fs?1:0);
cnt++;
if(cnt-(ls?1:0)-(fs?1:0)>s_mp&&t[0]!=0&&t[0]!=-1){
if(ls){
lc.className=(lc.className=="stxt")?"stxt2":"stxt";
}
tbl.deleteRow(cnt-1-(ls?1:0));
cnt--;
}
var nc=(fm!=null)?((fm.className=="stxt")?"2":""):"2";
op=tbl.rows[cnt-1-(ls?1:0)].id;
}else{
y=tbl.insertRow(cnt-(ls?1:0));
cnt++;
if(cnt-(ls?1:0)-(fs?1:0)>s_mp&&t[0]!=0&&t[0]!=-1){
if(fs){
fc.className=(fc.className=="stxt")?"stxt2":"stxt";
}
tbl.deleteRow(0+(fs?1:0));
cnt--;
}
var nc=(lm!=null)?((lm.className=="stxt")?"2":""):"2";
op=tbl.rows[fs?1:0].id;
}
}else{
y=tbl.insertRow(0);
var nc="2";
}
y.id=t[0];
z=y.insertCell(-1);
z.className="stxt"+nc;
x="";
if(t[0]==0||t[0]==-1){
x+="<div align=\"center\">"+t[6]+"</div>";
}else{
s=t[5].substring(t[5].length-4);
if(s_av&&(s==".gif"||s==".jpg"||s==".png")){
x+="<img src=\""+t[5]+"\" class=\"pic\">";
t[5]="";
}
if(s_dt>1){
x+="<div class=\"dtxt"+nc+"\" id=\"t"+t[1]+"\" "+((s_rt)?"dir=\"ltr\"":"")+">"+t[2]+"</div>";
}
if(t[0]==-2){
x+="<div class=\"dtxt"+nc+"\">("+t15+")</div>";
}
if(t[5]){
x+="<a href=\""+t[5]+"\" target=\"_blank\">";
}
x+="<b";
if(s_dt==1){
x+=" title=\""+t[2]+"\"";
}
var ncl="";
switch(t[4]){
case "1":
ncl="pn_std";
break;
case "2":
ncl="pn_reg";
break;
case "3":
ncl="pn_mod";
break;
case "4":
ncl="pn_adm";
break;
}
if(ncl){
x+=" class=\""+ncl+"\"";
}
if(s_rt){
x+=" dir=\"ltr\"";
}
x+=">";
x+=t[3];
x+="</b>";
if(t[5]){
x+="</a>";
}
x+=": "+t[6];
}
z.innerHTML=x;
parent.cboxmain.scrollTo(0,(!s_sd)?0:99999);
if(parent.cboxmain.document.getElementById("addiv")){
xy = parent.cboxmain.document;
ads=xy.createElement("script");
ads.type="text/javascript";
xx = xy.getElementById("addiv");
ads.src=xx.innerHTML;
xx.innerHTML = "";
xx.style.display = "";
xy.getElementsByTagName("head")[0].appendChild(ads); 
}
}
function upd_tms(c){
if(s_dt!=3){
return true;
}
if(c===undefined){
return true;
}
tarr=t16;
a=parent.cboxmain.document.getElementsByTagName("div");
ar=new Array(2592000,604800,86400,3600,60,1);
for(i=0;i<a.length;i++){
if(a[i].id.substring(0,1)=="t"){
d=c-a[i].id.substring(1);
if(d<0){
continue;
}
for(j=0;j<6;j++){
e=0;
if(d/ar[j]>1){
d=d/ar[j];
e=10-j*2;
break;
}
}
if(!a[i].title){
a[i].title=a[i].innerHTML;
}
d=Math.round(d);
a[i].innerHTML=(t24.replace("%d",d)).replace("%s",tarr[(d==1)?e:e+1]);
}
}
}
var lnkd=new Array();
function delban(){
var lvl=getlvl();
if(lvl>2){
mod=true;
}else{
mod=false;
}
cn=tbl.rows.length;
for(i=0;i<cn;i++){
a=tbl.rows[i];
if(a.id<1){
continue;
}
b=a.cells[0];
if(lnkd[a.id]){
lnkd[a.id]=false;
b.innerHTML=b.innerHTML.replace(/(div>|^)<span><a.*?del.*?a>&nbsp;<\/span></i,"$1<");
}
if((mod||(s_ld&&a.id==mylp&&mylp==lp&&lvl>1))){
if(!lnkd[a.id]){
lnkd[a.id]=true;
b.innerHTML=b.innerHTML.replace(/(div>|^)<(a|b|img)/i,"$1<span><a href=\"JavaScript:parent.cboxform.del("+a.id+")\" title=\""+t17+"\">[&times;]</a>&nbsp;"+((mod)?"<a href=\"JavaScript:parent.cboxform.ban("+a.id+")\" title=\""+t18+"\" onMouseOver=\"JavaScript:parent.cboxform.getip("+a.id+", this)\">[o]</a>&nbsp;":"")+"</span><$2");
}
}
}
}
function del(i){
if(confirm(t19)){
if(!http("GET","./?"+s_rq+"&sec=delban&n="+esc(f.nme.value)+"&k="+f.key.value+"&del="+i,null,"del_proc",false)){
alert(t20);
}
}
}
function ban(i){
var t=prompt(t21,"7 days");
if(t!=null){
if(!http("GET","./?"+s_rq+"&sec=delban&n="+esc(f.nme.value)+"&k="+f.key.value+"&ban="+i+"&dur="+esc(t),null,"ban_proc",false)){
alert(t20);
}
}
}
var gips=new Array();
function getip(i,k){
if(!gips[i]){
http("GET","./?"+s_rq+"&sec=getip&n="+esc(f.nme.value)+"&k="+f.key.value+"&i="+i+"&r="+((new Date()).getTime()),null,"getip_proc",true,k);
gips[i]=true;
}
}
function getip_proc(x,i){
if(!x){
return false;
}
if(i){
i.title=t18+" (IP: "+x+")";
}
}
function del_proc(x){
if(!x){
return false;
}
for(var i=0;i<tbl.rows.length;i++){
if(tbl.rows[i].id==x*1){
tbl.rows[i].cells[0].innerHTML="<div align=\"center\"><i>"+t22+"</i></div>";
}
}
}
function ban_proc(x){
if(!x){
return false;
}
x=x.split("\t");
alert(x[1]);
}
function selchk(e){
if(!e){
var e=window.event;
}
var f=e.srcElement.tagName.toLowerCase();
if(f!="textarea"&&f!="input"){
return false;
}else{
return true;
}
}

