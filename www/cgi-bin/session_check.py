#!/usr/bin/env python3
import os

print("Content-Type: text/html\r\n\r\n", end="")

cookie_header = os.environ.get("HTTP_COOKIE", "")
has_session   = "SESSION_ID" in cookie_header

# extract session id value if present
session_id = ""
if has_session:
    for part in cookie_header.split(";"):
        part = part.strip()
        if part.startswith("SESSION_ID="):
            session_id = part.split("=", 1)[1].strip()
            break
    if not session_id:
        session_id = "(present, no value)"

print(f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Session Checker · webserv</title>
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link href="https://fonts.googleapis.com/css2?family=Syne:wght@400;600;700;800&family=DM+Sans:ital,opsz,wght@0,9..40,300;0,9..40,400;0,9..40,500;1,9..40,300&family=JetBrains+Mono:wght@400;500&display=swap" rel="stylesheet">
  <style>
    :root {{
      --bg:      #04050f;
      --glass:   rgba(255,255,255,0.05);
      --glass-b: rgba(255,255,255,0.09);
      --teal:    #00f0d4;
      --violet:  #a78bfa;
      --rose:    #fb7185;
      --blue:    #60a5fa;
      --text:    #e2e8f0;
      --muted:   #475569;
      --muted2:  #94a3b8;
    }}

    *, *::before, *::after {{ box-sizing:border-box; margin:0; padding:0; }}
    html {{ scroll-behavior:smooth; }}

    body {{
      background: var(--bg);
      color: var(--text);
      font-family: 'DM Sans', sans-serif;
      min-height: 100vh;
      overflow-x: hidden;
      display: flex;
      flex-direction: column;
    }}

    /* ── aurora ── */
    .aurora {{ position:fixed; inset:0; z-index:0; pointer-events:none; overflow:hidden; }}
    .blob {{ position:absolute; border-radius:50%; filter:blur(120px); animation:drift 14s ease-in-out infinite; }}
    .b1 {{ width:800px;height:800px;background:radial-gradient(circle,rgba(167,139,250,.35),rgba(96,165,250,.2));top:-300px;left:-200px; }}
    .b2 {{ width:600px;height:600px;background:radial-gradient(circle,rgba(0,240,212,.25),rgba(96,165,250,.15));top:50%;right:-150px;animation-delay:-5s; }}
    .b3 {{ width:700px;height:700px;background:radial-gradient(circle,rgba(251,113,133,.2),rgba(167,139,250,.2));bottom:-200px;left:35%;animation-delay:-9s; }}

    @keyframes drift {{
      0%,100% {{ transform:translate(0,0) scale(1) }}
      33%      {{ transform:translate(50px,-40px) scale(1.06) }}
      66%      {{ transform:translate(-30px,30px) scale(.96) }}
    }}

    body::after {{
      content:''; position:fixed; inset:0; pointer-events:none; z-index:0;
      background-image:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg'%3E%3Cfilter id='n'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='0.85' numOctaves='4'/%3E%3C/filter%3E%3Crect width='100%25' height='100%25' filter='url(%23n)' opacity='0.035'/%3E%3C/svg%3E");
    }}

    /* ── nav ── */
    nav {{
      position:fixed; top:18px; left:50%; transform:translateX(-50%);
      width:calc(100% - 64px); max-width:1160px; z-index:200;
      display:flex; align-items:center; justify-content:space-between;
      padding:12px 24px; border-radius:18px;
      background:rgba(4,5,15,.65); backdrop-filter:blur(28px);
      border:1px solid var(--glass-b);
    }}
    .nav-logo {{
      font-family:'Syne',sans-serif; font-weight:800; font-size:18px;
      letter-spacing:-.03em; text-decoration:none;
      background:linear-gradient(130deg,var(--teal),var(--violet));
      -webkit-background-clip:text; -webkit-text-fill-color:transparent; background-clip:text;
    }}
    .nav-links {{ display:flex; gap:4px; list-style:none; }}
    .nav-links a {{
      color:var(--muted2); text-decoration:none; font-size:14px; font-weight:500;
      padding:7px 14px; border-radius:10px; transition:all .2s;
    }}
    .nav-links a:hover {{ color:#fff; background:var(--glass); }}
    .nav-pill {{
      font-size:12px; font-weight:600; color:var(--teal);
      background:rgba(0,240,212,.1); border:1px solid rgba(0,240,212,.2);
      padding:6px 16px; border-radius:30px; display:flex; align-items:center; gap:6px;
    }}
    .pulse {{
      width:7px; height:7px; background:var(--teal); border-radius:50%;
      box-shadow:0 0 10px var(--teal); animation:blink 2s ease infinite;
    }}
    @keyframes blink {{ 0%,100% {{ opacity:1 }} 50% {{ opacity:.35 }} }}

    /* ── page ── */
    .page {{
      flex:1; display:flex; align-items:center; justify-content:center;
      padding:120px 40px 80px; position:relative; z-index:1;
    }}

    .wrap {{
      width:100%; max-width:520px; text-align:center;
      animation:fadeU .6s ease both;
    }}

    @keyframes fadeU {{
      from {{ opacity:0; transform:translateY(18px) }}
      to   {{ opacity:1; transform:translateY(0) }}
    }}

    /* ── big status icon ── */
    .status-ring {{
      width:96px; height:96px; border-radius:28px; margin:0 auto 28px;
      display:flex; align-items:center; justify-content:center;
      position:relative;
    }}

    .status-ring.alive {{
      background:rgba(0,240,212,.08);
      border:1px solid rgba(0,240,212,.25);
      box-shadow:0 0 60px rgba(0,240,212,.18);
    }}

    .status-ring.dead {{
      background:rgba(251,113,133,.08);
      border:1px solid rgba(251,113,133,.25);
      box-shadow:0 0 60px rgba(251,113,133,.18);
    }}

    .status-ring svg {{
      width:42px; height:42px; fill:none; stroke-width:1.8;
      stroke-linecap:round; stroke-linejoin:round;
    }}
    .status-ring.alive svg {{ stroke:var(--teal); }}
    .status-ring.dead  svg {{ stroke:var(--rose); }}

    /* ping animation for alive */
    .status-ring.alive::before {{
      content:''; position:absolute; inset:-12px; border-radius:34px;
      border:1px solid rgba(0,240,212,.2);
      animation:ping 2.4s ease-out infinite;
    }}
    @keyframes ping {{
      0%   {{ opacity:.7; transform:scale(1) }}
      100% {{ opacity:0;  transform:scale(1.2) }}
    }}

    /* ── label row ── */
    .label-row {{
      font-size:11px; font-weight:700; letter-spacing:.22em; text-transform:uppercase;
      margin-bottom:14px; display:inline-flex; align-items:center; gap:10px;
    }}
    .label-row::before {{ content:''; width:22px; height:2px; border-radius:1px; }}
    .label-row.alive {{ color:var(--teal); }}
    .label-row.alive::before {{ background:var(--teal); }}
    .label-row.dead  {{ color:var(--rose); }}
    .label-row.dead::before  {{ background:var(--rose); }}

    /* ── title ── */
    .page-title {{
      font-family:'Syne',sans-serif; font-size:clamp(32px,5vw,52px);
      font-weight:800; letter-spacing:-.03em; line-height:1.05;
      color:#fff; margin-bottom:12px;
    }}
    .page-title .accent-alive {{
      background:linear-gradient(135deg,var(--teal),var(--blue));
      -webkit-background-clip:text; -webkit-text-fill-color:transparent; background-clip:text;
    }}
    .page-title .accent-dead {{
      background:linear-gradient(135deg,var(--rose),var(--violet));
      -webkit-background-clip:text; -webkit-text-fill-color:transparent; background-clip:text;
    }}

    .page-sub {{
      font-size:15px; color:var(--muted2); font-weight:300;
      line-height:1.8; margin-bottom:36px;
    }}

    /* ── session id card ── */
    .session-card {{
      background:rgba(0,240,212,.05);
      border:1px solid rgba(0,240,212,.18);
      border-radius:14px; padding:18px 22px;
      margin-bottom:28px; text-align:left;
      animation:fadeU .6s ease .1s both;
    }}

    .session-card-label {{
      font-size:11px; font-weight:700; letter-spacing:.18em; text-transform:uppercase;
      color:var(--teal); margin-bottom:8px;
    }}

    .session-id-value {{
      font-family:'JetBrains Mono',monospace; font-size:13px; font-weight:500;
      color:var(--text); word-break:break-all;
      display:flex; align-items:center; gap:10px;
    }}

    .sid-dot {{
      width:8px; height:8px; background:var(--teal); border-radius:50%;
      box-shadow:0 0 8px var(--teal); flex-shrink:0;
      animation:blink 2s ease infinite;
    }}

    /* ── no-session hint ── */
    .hint-card {{
      background:rgba(251,113,133,.05);
      border:1px solid rgba(251,113,133,.18);
      border-radius:14px; padding:18px 22px;
      margin-bottom:28px; text-align:left;
      animation:fadeU .6s ease .1s both;
      display:flex; align-items:flex-start; gap:14px;
    }}

    .hint-icon {{
      width:36px; height:36px; border-radius:10px; flex-shrink:0;
      background:rgba(251,113,133,.1); border:1px solid rgba(251,113,133,.2);
      display:flex; align-items:center; justify-content:center;
    }}
    .hint-icon svg {{ width:16px; height:16px; stroke:var(--rose); fill:none; stroke-width:2; stroke-linecap:round; stroke-linejoin:round; }}

    .hint-text {{ font-size:13px; color:var(--muted2); font-weight:300; line-height:1.7; }}
    .hint-text strong {{ color:var(--rose); font-weight:600; }}

    /* ── button ── */
    .btn-ghost {{
      font-family:'Syne',sans-serif; font-weight:600; font-size:14px;
      padding:13px 28px; border-radius:12px; cursor:pointer; text-decoration:none;
      display:inline-flex; align-items:center; justify-content:center; gap:8px;
      background:var(--glass); color:var(--text); border:1px solid var(--glass-b);
      backdrop-filter:blur(12px); transition:all .25s;
    }}
    .btn-ghost:hover {{
      background:rgba(255,255,255,.09); border-color:rgba(255,255,255,.18); transform:translateY(-3px);
    }}
    .btn-ghost svg {{ width:15px; height:15px; stroke:currentColor; fill:none; stroke-width:2; stroke-linecap:round; stroke-linejoin:round; }}

    /* ── footer ── */
    footer {{ position:relative; z-index:1; border-top:1px solid var(--glass-b); padding:36px 40px; }}
    .foot-inner {{ max-width:1160px; margin:0 auto; display:flex; justify-content:space-between; align-items:center; gap:20px; }}
    .foot-logo {{ font-family:'Syne',sans-serif; font-weight:800; font-size:16px; background:linear-gradient(130deg,var(--teal),var(--violet)); -webkit-background-clip:text; -webkit-text-fill-color:transparent; background-clip:text; }}
    .foot-copy {{ font-size:13px; color:var(--muted); }}
    .foot-links {{ display:flex; gap:20px; list-style:none; }}
    .foot-links a {{ font-size:13px; color:var(--muted2); text-decoration:none; transition:color .2s; }}
    .foot-links a:hover {{ color:#fff; }}
  </style>
</head>
<body>

  <div class="aurora">
    <div class="blob b1"></div>
    <div class="blob b2"></div>
    <div class="blob b3"></div>
  </div>

  <nav>
    <a class="nav-logo" href="/">webserv</a>
    <ul class="nav-links">
      <li><a href="/#about">About</a></li>
      <li><a href="/#features">Features</a></li>
      <li><a href="/#team">Team</a></li>
      <li><a href="/#pipeline">Pipeline</a></li>
    </ul>
    <div class="nav-pill"><div class="pulse"></div>HTTP/1.0 · C++98</div>
  </nav>

  <div class="page">
    <div class="wrap">

      {'<!-- ALIVE -->' if has_session else '<!-- DEAD -->'}

      <!-- status icon -->
      <div class="status-ring {'alive' if has_session else 'dead'}">
        {'<svg viewBox="0 0 24 24"><path d="M22 11.08V12a10 10 0 1 1-5.93-9.14"/><polyline points="22 4 12 14.01 9 11.01"/></svg>' if has_session else '<svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"/><line x1="12" y1="8" x2="12" y2="12"/><line x1="12" y1="16" x2="12.01" y2="16"/></svg>'}
      </div>

      <div class="label-row {'alive' if has_session else 'dead'}">
        CGI · Session Checker
      </div>

      <h1 class="page-title">
        {'Session is <span class="accent-alive">Alive.</span>' if has_session else 'No Session <span class="accent-dead">Found.</span>'}
      </h1>

      <p class="page-sub">
        {'A valid <code style="color:var(--teal);background:rgba(0,240,212,.1);padding:2px 8px;border-radius:5px;font-size:13px">SESSION_ID</code> cookie was detected in this request.' if has_session else 'No <code style="color:var(--rose);background:rgba(251,113,133,.1);padding:2px 8px;border-radius:5px;font-size:13px">SESSION_ID</code> cookie was found in the Cookie header.'}
      </p>

      {'<!-- session id card --><div class="session-card"><div class="session-card-label">SESSION_ID</div><div class="session-id-value"><div class="sid-dot"></div>' + session_id + '</div></div>' if has_session else '<!-- hint card --><div class="hint-card"><div class="hint-icon"><svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="10"/><line x1="12" y1="8" x2="12" y2="12"/><line x1="12" y1="16" x2="12.01" y2="16"/></svg></div><div class="hint-text">The server found no <strong>SESSION_ID</strong> in <code style="font-family:monospace;font-size:12px">HTTP_COOKIE</code>. Start a session to see it appear here.</div></div>'}

      <a href="/" class="btn-ghost">
        <svg viewBox="0 0 24 24"><polyline points="15 18 9 12 15 6"/></svg>
        Back to Home
      </a>

    </div>
  </div>

  <footer>
    <div class="foot-inner">
      <div class="foot-logo">webserv</div>
      <span class="foot-copy">© 2025 REDA, ELAMRANI, SIMO · 1337 School</span>
      <ul class="foot-links">
        <li><a href="/">Home</a></li>
        <li><a href="/404.html">404</a></li>
        <li><a href="/500.html">500</a></li>
      </ul>
    </div>
  </footer>

</body>
</html>""")