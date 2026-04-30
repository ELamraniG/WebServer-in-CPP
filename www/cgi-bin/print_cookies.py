#!/usr/bin/env python3
import os
import sys

sys.stdout.write("Content-Type: text/html\r\n\r\n")

raw = os.environ.get("HTTP_COOKIE", "")

if raw:
    pairs = []
    for part in raw.split(";"):
        part = part.strip()
        if "=" in part:
            k, v = part.split("=", 1)
            pairs.append((k.strip(), v.strip()))
        elif part:
            pairs.append((part, ""))
else:
    pairs = []

# ── build cookie cards ────────────────────────────────────────
def cookie_card(key, value, index):
    # cycle accent colors
    accents = [
        ("var(--teal)",   "rgba(0,240,212,0.08)",  "rgba(0,240,212,0.22)"),
        ("var(--violet)", "rgba(167,139,250,0.08)", "rgba(167,139,250,0.22)"),
        ("var(--blue)",   "rgba(96,165,250,0.08)",  "rgba(96,165,250,0.22)"),
        ("var(--rose)",   "rgba(251,113,133,0.08)", "rgba(251,113,133,0.22)"),
    ]
    color, bg, border = accents[index % len(accents)]
    delay = index * 0.07

    label = key if key else "(unnamed)"
    val   = value if value else "(empty)"

    return f"""
    <div class="cookie-card" style="
        animation-delay:{delay}s;
        background:{bg};
        border-color:{border};
    ">
      <div class="cookie-dot" style="background:{color};box-shadow:0 0 8px {color};"></div>
      <div class="cookie-body">
        <div class="cookie-key" style="color:{color}">{label}</div>
        <div class="cookie-val">{val}</div>
      </div>
      <div class="cookie-index" style="color:{color};opacity:.35">#{index+1:02d}</div>
    </div>"""

cards_html = ""
if pairs:
    for i, (k, v) in enumerate(pairs):
        cards_html += cookie_card(k, v, i)
else:
    cards_html = """
    <div class="empty-state">
      <div class="empty-icon">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor"
             stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
          <circle cx="12" cy="12" r="10"/>
          <path d="M8 15s1.5 2 4 2 4-2 4-2"/>
          <line x1="9" y1="9" x2="9.01" y2="9"/>
          <line x1="15" y1="9" x2="15.01" y2="9"/>
        </svg>
      </div>
      <div class="empty-title">No cookies found</div>
      <div class="empty-sub">This request carried no Cookie header.</div>
    </div>"""

count      = len(pairs)
count_label = f"{count} cookie{'s' if count != 1 else ''} found"

print(f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Cookie Inspector · webserv</title>
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

    *, *::before, *::after {{ box-sizing: border-box; margin: 0; padding: 0; }}
    html {{ scroll-behavior: smooth; }}

    body {{
      background: var(--bg);
      color: var(--text);
      font-family: 'DM Sans', sans-serif;
      min-height: 100vh;
      overflow-x: hidden;
    }}

    /* ── aurora ── */
    .aurora {{ position: fixed; inset: 0; z-index: 0; pointer-events: none; overflow: hidden; }}
    .blob {{ position: absolute; border-radius: 50%; filter: blur(120px); animation: drift 14s ease-in-out infinite; }}
    .b1 {{ width:800px;height:800px;background:radial-gradient(circle,rgba(167,139,250,.35),rgba(96,165,250,.2));top:-300px;left:-200px;animation-delay:0s; }}
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
      min-height:100vh; display:flex; align-items:flex-start; justify-content:center;
      padding:120px 40px 80px; position:relative; z-index:1;
    }}

    .wrap {{ width:100%; max-width:680px; }}

    /* ── header ── */
    .label-row {{
      font-size:11px; font-weight:700; letter-spacing:.22em; text-transform:uppercase;
      color:var(--teal); margin-bottom:14px; display:flex; align-items:center; gap:10px;
    }}
    .label-row::before {{ content:''; width:22px; height:2px; background:var(--teal); border-radius:1px; }}

    .page-title {{
      font-family:'Syne',sans-serif; font-size:clamp(36px,6vw,58px);
      font-weight:800; letter-spacing:-.03em; line-height:1.05; color:#fff; margin-bottom:10px;
    }}
    .page-title span {{
      background:linear-gradient(135deg,var(--teal),var(--violet));
      -webkit-background-clip:text; -webkit-text-fill-color:transparent; background-clip:text;
    }}
    .page-sub {{
      font-size:16px; color:var(--muted2); font-weight:300; margin-bottom:36px; line-height:1.8;
    }}

    /* ── stat bar ── */
    .stat-bar {{
      display:flex; align-items:center; justify-content:space-between;
      background:var(--glass); border:1px solid var(--glass-b);
      backdrop-filter:blur(20px); border-radius:14px;
      padding:14px 20px; margin-bottom:20px;
      animation: fadeU .5s ease both;
    }}
    .stat-left {{ display:flex; align-items:center; gap:10px; }}
    .stat-icon {{
      width:34px; height:34px; border-radius:9px;
      background:rgba(0,240,212,.1); border:1px solid rgba(0,240,212,.2);
      display:flex; align-items:center; justify-content:center;
    }}
    .stat-icon svg {{ width:16px; height:16px; stroke:var(--teal); fill:none; stroke-width:1.8; stroke-linecap:round; stroke-linejoin:round; }}
    .stat-text {{ font-family:'Syne',sans-serif; font-weight:700; font-size:14px; color:var(--text); }}
    .stat-sub  {{ font-size:12px; color:var(--muted2); margin-top:1px; }}
    .stat-badge {{
      font-family:'JetBrains Mono',monospace; font-size:13px; font-weight:500;
      color:var(--teal); background:rgba(0,240,212,.1);
      border:1px solid rgba(0,240,212,.2); padding:4px 14px; border-radius:30px;
    }}

    /* ── cookie cards ── */
    .cards {{ display:flex; flex-direction:column; gap:10px; }}

    .cookie-card {{
      display:flex; align-items:center; gap:16px;
      border:1px solid; border-radius:14px; padding:16px 20px;
      animation:fadeU .5s ease both;
      transition:transform .2s, box-shadow .2s;
      position:relative; overflow:hidden;
    }}

    .cookie-card::before {{
      content:''; position:absolute; inset:0;
      background:linear-gradient(120deg,rgba(255,255,255,.03) 0%,transparent 60%);
      pointer-events:none;
    }}

    .cookie-card:hover {{
      transform:translateX(4px);
      box-shadow:0 4px 30px rgba(0,0,0,.3);
    }}

    .cookie-dot {{
      width:8px; height:8px; border-radius:50%; flex-shrink:0;
    }}

    .cookie-body {{ flex:1; min-width:0; }}

    .cookie-key {{
      font-family:'JetBrains Mono',monospace; font-weight:500; font-size:13px;
      letter-spacing:.02em; margin-bottom:3px;
    }}

    .cookie-val {{
      font-family:'JetBrains Mono',monospace; font-size:12px;
      color:var(--muted2); white-space:nowrap; overflow:hidden; text-overflow:ellipsis;
    }}

    .cookie-index {{
      font-family:'JetBrains Mono',monospace; font-size:11px; font-weight:500;
      flex-shrink:0;
    }}

    /* ── empty state ── */
    .empty-state {{
      text-align:center; padding:60px 40px;
      background:var(--glass); border:1px solid var(--glass-b);
      border-radius:20px; animation:fadeU .5s ease both;
    }}
    .empty-icon {{
      width:56px; height:56px; margin:0 auto 20px;
      border-radius:16px; background:rgba(255,255,255,.05); border:1px solid var(--glass-b);
      display:flex; align-items:center; justify-content:center; color:var(--muted2);
    }}
    .empty-icon svg {{ width:26px; height:26px; }}
    .empty-title {{
      font-family:'Syne',sans-serif; font-weight:700; font-size:18px;
      color:var(--text); margin-bottom:8px;
    }}
    .empty-sub {{ font-size:14px; color:var(--muted2); font-weight:300; }}

    /* ── back button ── */
    .btn-ghost {{
      font-family:'Syne',sans-serif; font-weight:600; font-size:14px;
      padding:13px 28px; border-radius:12px; cursor:pointer; text-decoration:none;
      display:inline-flex; align-items:center; justify-content:center; gap:8px;
      background:var(--glass); color:var(--text); border:1px solid var(--glass-b);
      backdrop-filter:blur(12px); transition:all .25s; margin-top:20px;
    }}
    .btn-ghost:hover {{
      background:rgba(255,255,255,.09); border-color:rgba(255,255,255,.18); transform:translateY(-3px);
    }}
    .btn-ghost svg {{ width:15px; height:15px; stroke:currentColor; fill:none; stroke-width:2; stroke-linecap:round; stroke-linejoin:round; }}

    @keyframes fadeU {{
      from {{ opacity:0; transform:translateY(14px) }}
      to   {{ opacity:1; transform:translateY(0) }}
    }}

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

      <div class="label-row">CGI · Cookie Inspector</div>
      <h1 class="page-title">Request <span>Cookies.</span></h1>
      <p class="page-sub">
        All cookies sent with this HTTP request, parsed from the
        <code style="color:var(--teal);background:rgba(0,240,212,.1);padding:2px 8px;border-radius:5px;font-size:14px">Cookie</code> header via CGI environment.
      </p>

      <!-- stat bar -->
      <div class="stat-bar">
        <div class="stat-left">
          <div class="stat-icon">
            <svg viewBox="0 0 24 24">
              <circle cx="12" cy="12" r="10"/>
              <path d="M8.56 2.75c4.37 6.03 6.02 9.42 8.03 17.72m2.54-15.38c-3.72 4.35-8.94 5.66-16.88 5.85m19.5 1.9c-3.5-.93-6.63-.82-8.94 0-2.58.92-5.01 2.86-7.44 6.32"/>
            </svg>
          </div>
          <div>
            <div class="stat-text">HTTP_COOKIE</div>
            <div class="stat-sub">environment variable</div>
          </div>
        </div>
        <div class="stat-badge">{count_label}</div>
      </div>

      <!-- cards -->
      <div class="cards">
        {cards_html}
      </div>

      <a href="/" class="btn-ghost">
        <svg viewBox="0 0 24 24"><polyline points="15 18 9 12 15 6"/></svg>
        Back to Home
      </a>

    </div>
  </div>

  <footer>
    <div class="foot-inner">
      <div class="foot-logo">webserv</div>
      <span class="foot-copy">© 2025 REDA, ELAMRANI, SIMO · 42 School</span>
      <ul class="foot-links">
        <li><a href="/">Home</a></li>
        <li><a href="/404.html">404</a></li>
        <li><a href="/500.html">500</a></li>
      </ul>
    </div>
  </footer>

</body>
</html>""")