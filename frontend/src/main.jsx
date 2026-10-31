import React, { useEffect, useState } from 'react';
import { createRoot } from 'react-dom/client';
import { BusFront, Camera, Users, Activity } from 'lucide-react';
import './style.css';

const API = import.meta.env.VITE_API_URL || '/api';
const fmt = t => t ? new Date(t * 1000).toLocaleString('vi-VN') : 'Chưa có dữ liệu';
function App() {
  const [token, setToken] = useState(sessionStorage.getItem('token') || '');
  const [status, setStatus] = useState({online:false});
  const [events, setEvents] = useState([]);
  const [detections, setDetections] = useState([]);
  const [commands,setCommands]=useState([]);
  const [notice, setNotice] = useState('');
  const [busy, setBusy] = useState(false);
  const [connected, setConnected] = useState(false);
  async function api(path, options = {}) {
    const response = await fetch(`${API}${path}`, {...options, headers:{...options.headers, ...(token ? {Authorization:`Bearer ${token}`} : {})}});
    if (!response.ok) {
      if (response.status === 401 && path !== '/login') { sessionStorage.removeItem('token'); setToken(''); }
      const error = await response.json().catch(() => ({}));
      throw new Error(typeof error.detail === 'string' ? error.detail : `HTTP ${response.status}`);
    }
    return response.json();
  }
  async function load() {
    const [s,e,d,c] = await Promise.all([api('/status'),api('/events'),api('/detections'),api('/commands')]);
    setStatus(s); setEvents(e); setDetections(d); setCommands(c); setConnected(true);
  }
  useEffect(() => {
    if (!token) return;
    let active = true;
    async function poll() { try { await load(); } catch (error) { if(active) {setConnected(false); setNotice(error.message);} } }
    poll(); const timer = setInterval(poll,3000);
    const url = new URL(`${API}/ws`,location.origin); url.protocol=url.protocol==='https:'?'wss:':'ws:';
    const socket=new WebSocket(url);
    socket.onopen=()=>socket.send(JSON.stringify({token}));
    socket.onmessage=event=>{if(active) {setStatus(JSON.parse(event.data));setConnected(true);}};
    return () => {active=false; clearInterval(timer); socket.close();};
  },[token]);
  async function login(event) {
    event.preventDefault(); const form = new FormData(event.currentTarget);
    try {const data = await api('/login',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(Object.fromEntries(form))}); sessionStorage.setItem('token',data.access_token); setToken(data.access_token); setNotice('');}
    catch(error) {setNotice(error.message);}
  }
  async function action(path,body) {
    setBusy(true);
    try {const data = await api(path,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)}); setNotice(data.command_id ? `Đã gửi lệnh ${data.command_id}; chờ trạng thái thực tế từ mạch.` : 'Đã ghi nhận thông báo; vẫn cần kiểm tra và bấm nút cuối xe.'); await load();}
    catch(error) {setNotice(error.message);} finally {setBusy(false);}
  }
  async function upload(event) {
    const input=event.currentTarget; const file=input.files?.[0]; if(!file) return;
    setBusy(true);
    try {const body=new FormData(); body.append('file',file); const result=await api('/yolo/detect',{method:'POST',body}); await load(); setNotice(`YOLO phát hiện ${result.person_count} người`);}
    catch(error) {setNotice(error.message);} finally {setBusy(false); input.value='';}
  }
  const latest=detections[0]; const warning=status.state==='WARNING';
  return <div className="app"><header><div className="brand"><BusFront/><div><b>SMART BUS</b><span>GIÁM SÁT AN TOÀN</span></div></div>{token && <button onClick={()=>{sessionStorage.removeItem('token');setToken('');}}>Đăng xuất</button>}</header>
    {!token ? <main><form className="panel login" onSubmit={login}><h1>Đăng nhập quản trị</h1><label>Tài khoản<input name="username" required autoComplete="username"/></label><label>Mật khẩu<input type="password" name="password" required autoComplete="current-password"/></label><button>Đăng nhập</button><p role="alert">{notice}</p></form></main> : <main>
      <div className="title-row"><div><p className="eyebrow">TRUNG TÂM ĐIỀU HÀNH / BUS-01</p><h1>Giám sát an toàn hành khách</h1><p className="muted">API: {connected?'Đã kết nối':'Mất kết nối'} · ESP32: {connected&&status.online?'Trực tuyến':'Ngoại tuyến'}</p></div></div>
      <p role="status" className="notice">{notice}</p>
      {warning && <div className="alert"><div><strong>CẢNH BÁO CÒN NGƯỜI TRÊN XE</strong><p>{status.online&&connected?'Kiểm tra khoang xe ngay.':'Dữ liệu cảnh báo cuối cùng; mạch hiện không trực tuyến.'}</p></div><button disabled={busy} onClick={()=>action('/alarm/ack',{})}>Đã nhận thông báo</button></div>}
      <section className="cards"><article><Users/><span>SĨ SỐ CẢM BIẾN</span><strong>{status.passenger_count??'--'}</strong><small>Tổng lên: {status.total_boarded??0} · Tổng xuống: {status.total_alighted??0}<br/>{fmt(status.updated_at)}</small></article><article><Activity/><span>TRẠNG THÁI MẠCH</span><strong>{status.state||'OFFLINE'}</strong>{status.state==='CHECK'&&<p>Kiểm tra trong {status.check_remaining??0} giây</p>}<small>Khóa điện: {status.ignition_on?'ON':'OFF'} · PIR: {status.pir_ready?(status.pir_detected?'Chuyển động':'Không chuyển động'):'Chưa sẵn sàng'} · Quạt: {status.fan_on?'ON':'OFF'} · Còi: {status.buzzer_on?'ON':'OFF'}</small></article><article><Camera/><span>YOLO - KẾT QUẢ GẦN NHẤT</span><strong>{latest ? `${latest.person_count} người` : 'CHƯA CÓ'}</strong><small>{fmt(latest?.created_at)} · {latest?.source||'Chưa kết nối camera'}<br/>{latest&&latest.source==='camera'&&Date.now()/1000-latest.created_at<15?(latest.person_count===status.passenger_count?'AI và IR khớp':'AI và IR lệch - cần kiểm tra'):'Chưa có quan sát camera mới'}</small></article></section>
      <div className="grid"><section className="panel"><h2>Nhật ký sự kiện</h2><div className="events">{events.length?events.map(e=><div className="event" key={e.id}><div className={`dot ${e.severity}`}/><div><b>{e.message}</b><small>{e.type} · {fmt(e.created_at)}</small></div></div>):<p className="empty">Chưa có sự kiện.</p>}</div></section><section className="panel"><h2>Kiểm tra và điều khiển</h2><p className="muted">Lệnh quạt được gửi qua MQTT. FAN AUTO không tắt quạt khi mạch đang báo động.</p><div className="controls"><button disabled={busy||!status.online} onClick={()=>action('/commands',{device_id:'bus-01',action:'FAN_ON'})}>Bật quạt 5 phút</button><button disabled={busy||!status.online} onClick={()=>action('/commands',{device_id:'bus-01',action:'FAN_AUTO'})}>Quạt tự động</button></div><div className="controls"><button disabled={busy||!status.online} onClick={()=>action('/commands',{device_id:'bus-01',action:'BUZZER_SILENCE'})}>Tạm tắt còi 30 giây</button><button disabled={busy||!status.online} onClick={()=>action('/commands',{device_id:'bus-01',action:'BUZZER_AUTO'})}>Còi tự động</button></div>{commands.slice(0,5).map(c=><p className="muted" key={c.command_id}>{c.action}: {c.status==='applied'?'Mạch đã áp dụng':Date.now()/1000>c.expires_at?'Hết hạn / chưa có xác nhận':c.status}</p>)}<label className="upload">Phân tích ảnh bằng YOLO<input type="file" accept="image/jpeg,image/png" disabled={busy} onChange={upload}/></label><p className="muted">Dữ liệu được lưu MongoDB Atlas. Không dùng nút demo để giả dữ liệu mạch.</p></section></div>
    </main>}<footer>Đồ án AIoT · Smart Bus</footer></div>;
}
createRoot(document.getElementById('root')).render(<App/>);
