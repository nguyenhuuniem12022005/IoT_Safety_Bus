import React from 'react';

const ROWS = 8;
const SEAT_W = 44;
const SEAT_H = 26;
const FIRST_ROW_X = 150;
const ROW_STEP = 52;
const VERTICAL = 'matrix(0 1 -1 0 330 0)';

function Seat({ x, y }) {
  return <rect className="seat" x={x} y={y} width={SEAT_W} height={SEAT_H} rx="4" />;
}

function Label({ x, y, anchor = 'middle', label, detail, tone }) {
  return (
    <text className={`marker-text is-${tone}`} x={x} y={y} textAnchor={anchor}>
      <tspan className="marker-label">{label}</tspan>
      <tspan className="marker-detail" x={x} dy="15">
        {detail}
      </tspan>
    </text>
  );
}

function Plan({ vertical, alarm, checking, markers }) {
  const seats = [];
  for (let row = 0; row < ROWS; row += 1) {
    const x = FIRST_ROW_X + row * ROW_STEP;
    seats.push(<Seat key={`a${row}`} x={x} y={64} />, <Seat key={`b${row}`} x={x} y={134} />);
  }
  const place = vertical ? 'v' : 'h';

  return (
    <svg
      className={`cabin cabin-${place}${alarm ? ' is-alarm' : checking ? ' is-check' : ''}`}
      viewBox={vertical ? '0 0 400 700' : '0 -4 720 236'}
      role="img"
      aria-label="Sơ đồ khoang xe và vị trí cảm biến"
    >
      <g transform={vertical ? VERTICAL : undefined}>
        <path className="hull" d="M40 40 H650 Q680 40 680 70 V190 Q680 220 650 220 H40 Q20 220 20 200 V60 Q20 40 40 40 Z" />
        <line className="door-gap" x1="132" y1="40" x2="172" y2="40" />
        <line className="aisle" x1="140" y1="130" x2="566" y2="130" />
        {seats}
        <rect className="driver" x="40" y="134" width="44" height="40" rx="4" />
        <path className="sightline" d="M41 63 L108 130 H566" />
        {markers.map(m => (
          <rect key={m.id} className={`marker is-${m.tone}`} x={m.x - 10} y={m.y - 10} width="20" height="20" rx="4" />
        ))}
      </g>
      <text className="zone" x={vertical ? 176 : 62} y={vertical ? 102 : 196} textAnchor="middle">
        Tài xế
      </text>
      {markers.map(m => {
        const at = m[place];
        return (
          <g key={m.id}>
            {at.leader && <path className="leader" d={at.leader} />}
            <Label x={at.x} y={at.y} anchor={at.anchor} label={m.label} detail={m.detail} tone={m.tone} />
          </g>
        );
      })}
    </svg>
  );
}

export default function CabinPlan({ status, cameraFresh, cameraCount }) {
  const alarm = status.state === 'WARNING';
  const checking = status.state === 'CHECK';
  const markers = [
    {
      id: 'camera',
      x: 34,
      y: 56,
      tone: !cameraFresh ? 'idle' : cameraCount > 0 ? 'active' : 'ok',
      label: 'Camera',
      detail: cameraFresh ? `${cameraCount} người` : 'chưa có hình',
      h: { x: 24, y: 9, anchor: 'start' },
      v: { x: 304, y: 30, anchor: 'start' }
    },
    {
      id: 'ir',
      x: 152,
      y: 40,
      tone: (status.passenger_count ?? 0) > 0 ? 'active' : 'ok',
      label: 'IR cửa',
      detail: `${status.passenger_count ?? 0} người`,
      h: { x: 152, y: 9 },
      v: { x: 304, y: 148, anchor: 'start' }
    },
    {
      id: 'pir',
      x: 620,
      y: 110,
      tone: !status.pir_ready ? 'idle' : status.pir_detected ? 'alert' : 'ok',
      label: 'PIR',
      detail: !status.pir_ready ? 'khởi động' : status.pir_detected ? 'chuyển động' : 'yên',
      h: { x: 620, y: 72 },
      v: { x: 304, y: 616, anchor: 'start', leader: 'M230 620 H296' }
    },
    {
      id: 'button',
      x: 620,
      y: 190,
      tone: alarm ? 'alert' : checking ? 'warn' : 'idle',
      label: 'Nút cuối xe',
      detail: alarm || checking ? 'cần bấm tại xe' : 'không cần',
      h: { x: 620, y: 152 },
      v: { x: 96, y: 616, anchor: 'end', leader: 'M130 620 H104' }
    }
  ];

  return (
    <>
      <Plan alarm={alarm} checking={checking} markers={markers} />
      <Plan vertical alarm={alarm} checking={checking} markers={markers} />
    </>
  );
}
