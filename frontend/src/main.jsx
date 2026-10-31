import React, { useEffect, useRef, useState } from 'react';
import { createRoot } from 'react-dom/client';
import '@fontsource/be-vietnam-pro/400.css';
import '@fontsource/be-vietnam-pro/500.css';
import '@fontsource/be-vietnam-pro/600.css';
import '@fontsource/be-vietnam-pro/700.css';
import { Bell, BellOff, Fan, LogOut, VideoOff } from 'lucide-react';
import CabinPlan from './CabinPlan.jsx';
import CountHistory from './CountHistory.jsx';
import './style.css';

const API = import.meta.env.VITE_API_URL || '/api';
const DEVICE = 'bus-01';
const FRAME_STALE_S = 5;
const AI_EXPIRY_S = 15;
const CHECK_WINDOW_S = 15;

const clock = t => new Date(t * 1000).toLocaleTimeString('vi-VN', { hour: '2-digit', minute: '2-digit', second: '2-digit' });
const stamp = t => (t ? new Date(t * 1000).toLocaleString('vi-VN') : '—');

function ago(t, now) {
  if (!t) return 'chưa có dữ liệu';
  const s = Math.max(0, Math.round(now - t));
  if (s < 2) return 'vừa xong';
  if (s < 60) return `${s} giây trước`;
  if (s < 3600) return `${Math.floor(s / 60)} phút trước`;
  return `lúc ${clock(t)}`;
}

function lasted(t, now) {
  const s = Math.max(0, Math.round(now - t));
  if (s < 60) return `${s} giây`;
  const m = Math.floor(s / 60);
  return m < 60 ? `${m} phút ${s % 60} giây` : `${Math.floor(m / 60)} giờ ${m % 60} phút`;
}

const EVENT_TYPES = {
  ALARM: 'Cảnh báo',
  SAFETY_CHECK: 'Kiểm tra',
  INSPECTION: 'Nút cuối xe',
  ACKNOWLEDGED: 'Đã nhận',
  PASSENGER_COUNT: 'Sĩ số',
  YOLO_DETECTION: 'Camera',
  COMMAND: 'Lệnh'
};

const COMMANDS = {
  FAN_ON: 'Bật quạt 5 phút',
  FAN_AUTO: 'Quạt về tự động',
  BUZZER_SILENCE: 'Tắt còi 30 giây',
  BUZZER_AUTO: 'Còi về tự động'
};

function commandState(c, now) {
  if (c.status === 'applied') return ['applied', 'Mạch đã áp dụng'];
  if (c.status === 'failed') return ['failed', 'Gửi lỗi'];
  if (now > c.expires_at) return ['expired', 'Hết hạn, mạch chưa xác nhận'];
  return ['sent', 'Đã gửi, chờ mạch'];
}

function friendly(error) {
  const code = error.status;
  if (code === 401) return 'Sai tài khoản hoặc mật khẩu.';
  if (code === 502 || code === 504 || error.name === 'TypeError')
    return 'Không kết nối được máy chủ giám sát. Báo bộ phận kỹ thuật.';
  if (code === 503) return error.message || 'Máy chủ tạm thời không sẵn sàng.';
  return error.message;
}

function useNow() {
  const [now, setNow] = useState(() => Date.now() / 1000);
  useEffect(() => {
    const timer = setInterval(() => setNow(Date.now() / 1000), 1000);
    return () => clearInterval(timer);
  }, []);
  return now;
}

function Link({ on, label, detail }) {
  return (
    <span className={`link${on ? ' is-on' : ''}`}>
      <i aria-hidden="true" />
      {label}
      <span className="link-state">{on ? detail[0] : detail[1]}</span>
    </span>
  );
}

function Plate() {
  return <span className="plate">BUS-01</span>;
}

function SafetyStrip({ status, connected, alarmSince, now, busy, onAck }) {
  const online = connected && status.online;
  const remaining = status.check_remaining ?? 0;
  let tone = 'normal';
  let label = 'Bình thường';
  let message = `Khóa điện ${status.ignition_on ? 'bật' : 'tắt'} · ${status.passenger_count ?? 0} người trên xe theo IR`;

  if (!connected) {
    tone = 'offline';
    label = 'Mất kết nối';
    message = 'Không lấy được dữ liệu từ máy chủ';
  } else if (status.state === 'WARNING') {
    tone = 'warning';
    label = 'Còn người trên xe';
    const since = alarmSince ? `Báo động từ ${clock(alarmSince)}, đã ${lasted(alarmSince, now)} · ` : '';
    message = online
      ? `${since}Cử người kiểm tra khoang xe ngay`
      : `${since}Dữ liệu cuối ${ago(status.updated_at, now)}; mạch đang ngoại tuyến`;
  } else if (status.state === 'CHECK') {
    tone = 'check';
    label = 'Đang kiểm tra khoang xe';
    message = `Còn ${remaining} giây trước khi báo động`;
  } else if (!status.online) {
    tone = 'offline';
    label = 'Mạch ngoại tuyến';
    message = status.updated_at ? `Dữ liệu cuối ${ago(status.updated_at, now)}` : 'Chưa nhận dữ liệu từ mạch';
  }

  return (
    <div className={`strip is-${tone}`} role={tone === 'warning' ? 'alert' : 'status'}>
      <strong className="strip-label">{label}</strong>
      <span className="strip-message">{message}</span>
      <span className="tape" aria-hidden="true">
        {tone === 'check' && <span className="tape-run" style={{ width: `${(remaining / CHECK_WINDOW_S) * 100}%` }} />}
        {tone === 'warning' && <span className="tape-run" key="warning" />}
      </span>
      {tone === 'warning' && (
        <button className="btn on-strip" type="button" disabled={busy} onClick={onAck}>
          Đã nhận cảnh báo
        </button>
      )}
    </div>
  );
}

function CameraWindow({ frame, latest, cameraFresh, now }) {
  const age = frame ? now - frame.capturedAt : null;
  const stale = frame && age > FRAME_STALE_S;
  const conf = latest && latest.confidence ? Math.round(latest.confidence * 100) : null;
  const validFor = cameraFresh ? Math.max(0, Math.round(AI_EXPIRY_S - (now - latest.created_at))) : 0;

  return (
    <section className="glass" aria-labelledby="cam-title">
      <header className="glass-head">
        <h2 id="cam-title">Camera khoang xe</h2>
        <span>{frame ? `YOLOv8 · ${ago(frame.capturedAt, now)}` : 'YOLOv8 · chưa có hình'}</span>
      </header>
      <div className={`frame${stale ? ' is-stale' : ''}`}>
        {frame ? (
          <>
            <img src={frame.url} alt={`Hình camera khoang xe, YOLO đếm ${frame.count} người`} />
            {stale && (
              <span className="frame-stale">Hình cũ {Math.round(age)} giây · camera mất tín hiệu</span>
            )}
          </>
        ) : (
          <div className="frame-empty">
            <VideoOff size={28} strokeWidth={1.5} aria-hidden="true" />
            <p>Chưa nhận hình từ camera khoang xe.</p>
            <p>
              Máy chạy camera chưa kết nối. Báo bộ phận kỹ thuật kiểm tra. Khi có tín hiệu, hình có khung người
              do YOLO nhận diện sẽ hiện ở đây, mỗi giây một lần.
            </p>
          </div>
        )}
      </div>
      <dl className="readout">
        <div>
          <dt>Khung hình lúc</dt>
          <dd className={`num${frame ? '' : ' is-none'}`}>{frame ? clock(frame.capturedAt) : 'chưa có'}</dd>
        </div>
        <div>
          <dt>Độ tin cậy YOLO</dt>
          <dd className={`num${cameraFresh && conf !== null && latest.person_count > 0 ? '' : ' is-none'}`}>
            {!cameraFresh ? 'chưa có' : conf !== null && latest.person_count > 0 ? `${conf}%` : 'không thấy người'}
          </dd>
        </div>
        <div>
          <dt>Kết quả gửi xuống mạch</dt>
          <dd className={`num${cameraFresh ? '' : ' is-none'}`}>
            {cameraFresh ? `còn hiệu lực ${validFor} giây` : 'hết hiệu lực'}
          </dd>
        </div>
      </dl>
    </section>
  );
}

function OnBoard({ status, latest, cameraFresh, now }) {
  const ir = status.passenger_count ?? 0;
  let verdict = ['idle', 'Chưa có quan sát camera mới để đối chiếu'];
  if (cameraFresh) {
    const diff = latest.person_count - ir;
    verdict = diff === 0 ? ['ok', 'IR và camera khớp'] : ['warn', `Lệch ${Math.abs(diff)} người, cần kiểm tra`];
  }
  return (
    <section className="panel" aria-labelledby="onboard-title">
      <header className="panel-head">
        <h2 id="onboard-title">Ai đang trên xe</h2>
      </header>
      <table className="sources">
        <thead>
          <tr>
            <th scope="col">Nguồn</th>
            <th scope="col" className="value">Quan sát</th>
          </tr>
        </thead>
        <tbody>
          <tr>
            <th scope="row">
              Cảm biến IR ở cửa
              <small>{ago(status.updated_at, now)}</small>
            </th>
            <td className="value num">{status.updated_at ? `${ir} người` : '—'}</td>
          </tr>
          <tr>
            <th scope="row">
              Camera YOLO
              <small>{latest ? (cameraFresh ? ago(latest.created_at, now) : 'kết quả đã hết hạn') : 'chưa có kết quả'}</small>
            </th>
            <td className="value num">{cameraFresh ? `${latest.person_count} người` : '—'}</td>
          </tr>
          <tr>
            <th scope="row">
              PIR cuối xe
              <small>{!status.pir_ready ? 'đang khởi động' : ago(status.updated_at, now)}</small>
            </th>
            <td className={`value${status.pir_detected ? ' is-alert' : ''}`}>
              {!status.pir_ready ? '—' : status.pir_detected ? 'Có chuyển động' : 'Yên'}
            </td>
          </tr>
        </tbody>
      </table>
      <p className={`verdict is-${verdict[0]}`}>{verdict[1]}</p>
    </section>
  );
}

function Controls({ status, online, busy, commands, now, feedback, send }) {
  const offline = !online;
  return (
    <section className="panel" aria-labelledby="ctl-title">
      <header className="panel-head">
        <h2 id="ctl-title">Điều khiển từ xa</h2>
      </header>
      <div className="control">
        <div className="control-name">
          <Fan size={18} strokeWidth={1.75} aria-hidden="true" />
          Quạt <span className={status.fan_on ? 'state-on' : 'state-off'}>{status.fan_on ? 'đang chạy' : 'tắt'}</span>
        </div>
        <div className="control-actions">
          <button className="btn" type="button" disabled={busy || offline} onClick={() => send('FAN_ON')}>
            Bật 5 phút
          </button>
          <button className="btn" type="button" disabled={busy || offline} onClick={() => send('FAN_AUTO')}>
            Tự động
          </button>
        </div>
      </div>
      <div className="control">
        <div className="control-name">
          {status.buzzer_on ? (
            <Bell size={18} strokeWidth={1.75} aria-hidden="true" />
          ) : (
            <BellOff size={18} strokeWidth={1.75} aria-hidden="true" />
          )}
          Còi <span className={status.buzzer_on ? 'state-alert' : 'state-off'}>{status.buzzer_on ? 'đang kêu' : 'tắt'}</span>
        </div>
        <div className="control-actions">
          <button className="btn" type="button" disabled={busy || offline} onClick={() => send('BUZZER_SILENCE')}>
            Tắt 30 giây
          </button>
          <button className="btn" type="button" disabled={busy || offline} onClick={() => send('BUZZER_AUTO')}>
            Tự động
          </button>
        </div>
      </div>
      {offline && <p className="hint">Mạch ngoại tuyến nên chưa gửi được lệnh.</p>}
      {feedback && (
        <p className={`hint${feedback.error ? ' is-error' : ''}`} role="status">
          {feedback.text}
        </p>
      )}
      <h3 className="sub">Lệnh gần đây</h3>
      {commands.length ? (
        <ul className="commands">
          {commands.slice(0, 5).map(c => {
            const [key, text] = commandState(c, now);
            return (
              <li key={c.command_id} className={`is-${key}`}>
                <span className="cmd-name">{COMMANDS[c.action] || c.action}</span>
                <span className={`cmd is-${key}`}>{text}</span>
              </li>
            );
          })}
        </ul>
      ) : (
        <p className="hint">Chưa gửi lệnh nào trong ca này.</p>
      )}
    </section>
  );
}

function Events({ events }) {
  return (
    <section className="panel events" aria-labelledby="ev-title">
      <header className="panel-head">
        <h2 id="ev-title">Nhật ký sự kiện</h2>
        <span>{events.length} gần nhất</span>
      </header>
      {events.length ? (
        <div className="table-scroll">
          <table className="log">
            <thead>
              <tr>
                <th scope="col">Thời gian</th>
                <th scope="col">Loại</th>
                <th scope="col">Nội dung</th>
              </tr>
            </thead>
            <tbody>
              {events.map(e => (
                <tr key={e.id}>
                  <td className="num">{stamp(e.created_at)}</td>
                  <td>
                    <span className={`sev is-${e.severity}`}>{EVENT_TYPES[e.type] || e.type}</span>
                  </td>
                  <td>{e.type === 'COMMAND' ? COMMANDS[e.message] || e.message : e.message}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      ) : (
        <p className="empty">
          Chưa có sự kiện. Lên/xuống xe, tắt khóa điện và cảnh báo sẽ được ghi ở đây.
        </p>
      )}
    </section>
  );
}

function Login({ onLogin, error, busy }) {
  return (
    <main className="login">
      <form className="panel login-form" onSubmit={onLogin}>
        <h1>Đăng nhập ca trực</h1>
        <p className="hint">Dùng tài khoản quản trị trong cấu hình backend.</p>
        <label>
          Tài khoản
          <input name="username" required autoComplete="username" />
        </label>
        <label>
          Mật khẩu
          <input type="password" name="password" required autoComplete="current-password" />
        </label>
        <button className="btn primary" type="submit" disabled={busy}>
          {busy ? 'Đang đăng nhập…' : 'Vào ca trực'}
        </button>
        {error && (
          <p className="hint is-error" role="alert">
            {error}
          </p>
        )}
      </form>
    </main>
  );
}

function App() {
  const [token, setToken] = useState(sessionStorage.getItem('token') || '');
  const [status, setStatus] = useState({ online: false });
  const [events, setEvents] = useState([]);
  const [detections, setDetections] = useState([]);
  const [commands, setCommands] = useState([]);
  const [history, setHistory] = useState([]);
  const [frame, setFrame] = useState(null);
  const [connected, setConnected] = useState(false);
  const [loaded, setLoaded] = useState(false);
  const [busy, setBusy] = useState(false);
  const [feedback, setFeedback] = useState(null);
  const [loginError, setLoginError] = useState('');
  const frameUrl = useRef(null);
  const now = useNow();

  function signOut() {
    sessionStorage.removeItem('token');
    setToken('');
    setConnected(false);
    setLoaded(false);
    setFeedback(null);
  }

  async function api(path, options = {}) {
    const response = await fetch(`${API}${path}`, {
      ...options,
      headers: { ...options.headers, ...(token ? { Authorization: `Bearer ${token}` } : {}) }
    });
    if (!response.ok) {
      if (response.status === 401 && path !== '/login') signOut();
      const body = await response.json().catch(() => ({}));
      const error = new Error(typeof body.detail === 'string' ? body.detail : `HTTP ${response.status}`);
      error.status = response.status;
      throw error;
    }
    return response;
  }

  async function load() {
    const [s, e, d, c, h] = await Promise.all(
      ['/status', '/events', '/detections', '/commands', '/history?limit=500'].map(p => api(p).then(r => r.json()))
    );
    setStatus(s);
    setEvents(e);
    setDetections(d);
    setCommands(c);
    setHistory(h);
    setConnected(true);
    setLoaded(true);
  }

  useEffect(() => {
    if (!token) return;
    let active = true;
    async function poll() {
      try {
        await load();
      } catch {
        if (active) {
          setConnected(false);
          setLoaded(true);
        }
      }
    }
    poll();
    const timer = setInterval(poll, 3000);
    const url = new URL(`${API}/ws`, location.origin);
    url.protocol = url.protocol === 'https:' ? 'wss:' : 'ws:';
    const socket = new WebSocket(url);
    socket.onopen = () => socket.send(JSON.stringify({ token }));
    socket.onmessage = event => {
      if (active) {
        setStatus(JSON.parse(event.data));
        setConnected(true);
      }
    };
    return () => {
      active = false;
      clearInterval(timer);
      socket.close();
    };
  }, [token]);

  useEffect(() => {
    if (!token) return;
    let active = true;
    async function pull() {
      try {
        const response = await api(`/camera/frame?device_id=${DEVICE}`);
        const blob = await response.blob();
        if (!active) return;
        const url = URL.createObjectURL(blob);
        if (frameUrl.current) URL.revokeObjectURL(frameUrl.current);
        frameUrl.current = url;
        setFrame({
          url,
          capturedAt: Number(response.headers.get('X-Captured-At')) || Date.now() / 1000,
          count: Number(response.headers.get('X-Person-Count')) || 0
        });
      } catch (error) {
        if (active && error.status === 404) setFrame(null);
      }
    }
    pull();
    const timer = setInterval(pull, 1000);
    return () => {
      active = false;
      clearInterval(timer);
    };
  }, [token]);

  async function login(event) {
    event.preventDefault();
    const form = new FormData(event.currentTarget);
    setBusy(true);
    try {
      const response = await api('/login', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(Object.fromEntries(form))
      });
      const data = await response.json();
      sessionStorage.setItem('token', data.access_token);
      setToken(data.access_token);
      setLoginError('');
    } catch (error) {
      setLoginError(friendly(error));
    } finally {
      setBusy(false);
    }
  }

  async function post(path, body, success) {
    setBusy(true);
    try {
      await api(path, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(body)
      });
      setFeedback({ text: success });
      await load();
    } catch (error) {
      setFeedback({ text: friendly(error), error: true });
    } finally {
      setBusy(false);
    }
  }

  const send = action =>
    post('/commands', { device_id: DEVICE, action }, `Đã gửi “${COMMANDS[action]}”. Chờ mạch xác nhận.`);
  const ack = () =>
    post('/alarm/ack', {}, 'Đã ghi nhận. Cảnh báo chỉ tắt khi có người bấm nút kiểm tra cuối xe.');

  const latest = detections[0];
  const cameraFresh = Boolean(latest && now - latest.created_at < AI_EXPIRY_S);
  const online = connected && status.online;
  const cameraLive = Boolean(frame && now - frame.capturedAt <= FRAME_STALE_S);

  return (
    <div className="app">
      <header className="livery">
        <div className="brand">
          <Plate />
          <span className="brand-name">Smart Bus</span>
          <span className="brand-role">Bàn trực an toàn</span>
        </div>
        {token && (
          <div className="links">
            <Link on={connected} label="Máy chủ" detail={['kết nối', 'mất kết nối']} />
            <Link on={online} label="Mạch ESP32" detail={['trực tuyến', 'ngoại tuyến']} />
            <Link on={cameraLive} label="Camera" detail={['có hình', 'không có hình']} />
            <button className="btn ghost" type="button" onClick={signOut}>
              <LogOut size={16} strokeWidth={1.75} aria-hidden="true" />
              Đăng xuất
            </button>
          </div>
        )}
      </header>

      {!token ? (
        <Login onLogin={login} error={loginError} busy={busy} />
      ) : (
        <>
          {loaded ? (
            <SafetyStrip
              status={status}
              connected={connected}
              alarmSince={events.find(e => e.type === 'ALARM')?.created_at}
              now={now}
              busy={busy}
              onAck={ack}
            />
          ) : (
            <div className="strip is-loading" aria-hidden="true" />
          )}
          <main className="desk">
            <div className="desk-main">
              <CameraWindow frame={frame} latest={latest} cameraFresh={cameraFresh} now={now} />
              <section className="panel cabin-panel" aria-labelledby="cabin-title">
                <header className="panel-head">
                  <h2 id="cabin-title">Khoang xe</h2>
                  <span>Vị trí cảm biến trên xe</span>
                </header>
                <CabinPlan status={status} cameraFresh={cameraFresh} cameraCount={latest?.person_count ?? 0} />
                <ul className="legend" aria-label="Ý nghĩa màu cảm biến">
                  <li className="is-ok"><i />Bình thường, không thấy người</li>
                  <li className="is-active"><i />Đang thấy người</li>
                  <li className="is-warn"><i />Đang kiểm tra</li>
                  <li className="is-alert"><i />Báo động / cần xử lý</li>
                  <li><i />Chưa có dữ liệu</li>
                </ul>
              </section>
            </div>
            <div className="desk-side">
              <OnBoard status={status} latest={latest} cameraFresh={cameraFresh} now={now} />
              <Controls
                status={status}
                online={online}
                busy={busy}
                commands={commands}
                now={now}
                feedback={feedback}
                send={send}
              />
              <section className="panel" aria-labelledby="hist-title">
                <header className="panel-head">
                  <h2 id="hist-title">Sĩ số theo thời gian</h2>
                  <span>Theo cảm biến IR</span>
                </header>
                <CountHistory history={history} />
              </section>
            </div>
            <Events events={events} />
          </main>
        </>
      )}
    </div>
  );
}

createRoot(document.getElementById('root')).render(<App />);
