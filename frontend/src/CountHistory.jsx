import React, { useEffect, useRef, useState } from 'react';

const H = 160;
const PAD = { top: 30, right: 12, bottom: 26, left: 32 };

const hhmm = t => new Date(t * 1000).toLocaleTimeString('vi-VN', { hour: '2-digit', minute: '2-digit' });

export default function CountHistory({ history }) {
  const box = useRef(null);
  const [W, setW] = useState(640);
  useEffect(() => {
    if (!box.current) return undefined;
    const observer = new ResizeObserver(([entry]) => setW(Math.max(240, Math.round(entry.contentRect.width))));
    observer.observe(box.current);
    return () => observer.disconnect();
  }, []);

  const points = [...history].reverse();
  return (
    <div className="history-box" ref={box}>
      {points.length < 2 ? (
        <p className="empty">Chưa đủ telemetry. Biểu đồ sĩ số hiện khi mạch gửi dữ liệu.</p>
      ) : (
        <Chart points={points} W={W} />
      )}
    </div>
  );
}

function Chart({ points, W }) {

  const t0 = points[0].created_at;
  const t1 = points[points.length - 1].created_at;
  const span = Math.max(1, t1 - t0);
  const maxCount = Math.max(2, ...points.map(p => p.passenger_count ?? 0));
  const x = t => PAD.left + ((t - t0) / span) * (W - PAD.left - PAD.right);
  const y = c => PAD.top + (1 - c / maxCount) * (H - PAD.top - PAD.bottom);

  let d = `M${x(points[0].created_at)} ${y(points[0].passenger_count ?? 0)}`;
  for (let i = 1; i < points.length; i += 1) {
    d += ` H${x(points[i].created_at)} V${y(points[i].passenger_count ?? 0)}`;
  }

  const ignitionOff = [];
  for (let i = 1; i < points.length; i += 1) {
    if (points[i - 1].ignition_on && !points[i].ignition_on) ignitionOff.push(points[i].created_at);
  }
  const last = points[points.length - 1];

  return (
    <svg className="history" viewBox={`0 0 ${W} ${H}`} width={W} height={H} role="img" aria-label="Sĩ số theo thời gian">
      {[0, maxCount].map(v => (
        <g key={v}>
          <line className="grid" x1={PAD.left} x2={W - PAD.right} y1={y(v)} y2={y(v)} />
          <text className="axis" x={PAD.left - 8} y={y(v) + 4} textAnchor="end">{v}</text>
        </g>
      ))}
      {ignitionOff.map(t => (
        <g key={t} className="onset">
          <line x1={x(t)} x2={x(t)} y1={PAD.top - 18} y2={H - PAD.bottom} />
          {x(t) > W - 140 ? (
            <text x={x(t) - 5} y={14} textAnchor="end">Tắt khóa điện {hhmm(t)}</text>
          ) : (
            <text x={x(t) + 5} y={14}>Tắt khóa điện {hhmm(t)}</text>
          )}
        </g>
      ))}
      <path className="line" d={d} />
      <circle className="now" cx={x(last.created_at)} cy={y(last.passenger_count ?? 0)} r="4" />
      <text className="axis" x={PAD.left} y={H - 6}>{hhmm(t0)}</text>
      <text className="axis" x={W - PAD.right} y={H - 6} textAnchor="end">{hhmm(t1)}</text>
    </svg>
  );
}
