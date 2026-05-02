#!/usr/bin/php-cgi
<?php header("Content-Type: text/html"); ?>
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>PHP CGI · webserv</title>
  <link href="https://fonts.googleapis.com/css2?family=Syne:wght@700;800&family=DM+Sans:wght@300;400;500&family=JetBrains+Mono:wght@400;500&display=swap" rel="stylesheet">
  <style>
    *,*::before,*::after{box-sizing:border-box;margin:0;padding:0}
    body{background:#04050f;color:#e2e8f0;font-family:'DM Sans',sans-serif;min-height:100vh;overflow-x:hidden;padding-bottom:60px}
    .bg{position:fixed;inset:0;z-index:0;overflow:hidden}
    .bg::before{content:'';position:absolute;width:600px;height:600px;background:radial-gradient(circle,rgba(167,139,250,.2),transparent);filter:blur(100px);top:-150px;left:-100px}
    .bg::after{content:'';position:absolute;width:500px;height:500px;background:radial-gradient(circle,rgba(0,240,212,.14),transparent);filter:blur(100px);bottom:-100px;right:-50px}
    body::after{content:'';position:fixed;inset:0;background-image:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg'%3E%3Cfilter id='n'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='0.85' numOctaves='4'/%3E%3C/filter%3E%3Crect width='100%25' height='100%25' filter='url(%23n)' opacity='0.035'/%3E%3C/svg%3E");pointer-events:none;z-index:0}

    nav{position:fixed;top:18px;left:50%;transform:translateX(-50%);width:calc(100% - 64px);max-width:1000px;z-index:200;display:flex;align-items:center;justify-content:space-between;padding:12px 24px;border-radius:18px;background:rgba(4,5,15,.65);backdrop-filter:blur(28px);border:1px solid rgba(255,255,255,.09)}
    .nav-logo{font-family:'Syne',sans-serif;font-weight:800;font-size:18px;letter-spacing:-.03em;text-decoration:none;background:linear-gradient(130deg,#00f0d4,#a78bfa);-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text}
    .nav-links{display:flex;gap:4px;list-style:none}
    .nav-links a{color:#94a3b8;text-decoration:none;font-size:14px;font-weight:500;padding:7px 14px;border-radius:10px;transition:all .2s}
    .nav-links a:hover{color:#fff;background:rgba(255,255,255,.05)}
    .nav-pill{font-size:12px;font-weight:600;color:#a78bfa;background:rgba(167,139,250,.1);border:1px solid rgba(167,139,250,.2);padding:6px 16px;border-radius:30px;display:flex;align-items:center;gap:6px}
    .pulse{width:7px;height:7px;background:#a78bfa;border-radius:50%;box-shadow:0 0 10px #a78bfa;animation:blink 2s ease infinite}
    @keyframes blink{0%,100%{opacity:1}50%{opacity:.35}}

    .page{position:relative;z-index:1;max-width:900px;margin:0 auto;padding:110px 40px 0}

    .label{font-size:11px;font-weight:700;letter-spacing:.2em;text-transform:uppercase;color:#a78bfa;margin-bottom:10px;display:flex;align-items:center;gap:8px}
    .label::before{content:'';width:18px;height:2px;background:#a78bfa;border-radius:1px}
    h1{font-family:'Syne',sans-serif;font-size:clamp(28px,5vw,48px);font-weight:800;letter-spacing:-.03em;margin-bottom:6px;background:linear-gradient(135deg,#a78bfa,#00f0d4);-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text}
    .sub{font-size:14px;color:#475569;margin-bottom:36px;font-weight:300}

    /* ── stat chips ── */
    .chips{display:flex;flex-wrap:wrap;gap:8px;margin-bottom:32px}
    .chip{display:inline-flex;align-items:center;gap:8px;background:rgba(255,255,255,.04);border:1px solid rgba(255,255,255,.09);border-radius:30px;padding:7px 16px;font-size:12px;font-weight:500}
    .chip-dot{width:6px;height:6px;border-radius:50%;flex-shrink:0}
    .chip-key{color:#94a3b8}
    .chip-val{font-family:'JetBrains Mono',monospace;color:#e2e8f0;font-size:12px}

    /* ── section card ── */
    .card{background:rgba(255,255,255,.04);border:1px solid rgba(255,255,255,.09);border-radius:16px;overflow:hidden;margin-bottom:16px}
    .card-head{display:flex;align-items:center;gap:12px;padding:14px 20px;border-bottom:1px solid rgba(255,255,255,.07);background:rgba(255,255,255,.02)}
    .card-head-icon{width:30px;height:30px;border-radius:8px;display:flex;align-items:center;justify-content:center;flex-shrink:0}
    .card-head-icon svg{width:14px;height:14px;fill:none;stroke-width:1.8;stroke-linecap:round;stroke-linejoin:round}
    .card-title{font-family:'Syne',sans-serif;font-weight:700;font-size:13px;letter-spacing:.02em}
    .card-count{font-family:'JetBrains Mono',monospace;font-size:11px;color:#475569;margin-left:auto}

    /* ── env table ── */
    .env-table{width:100%;border-collapse:collapse}
    .env-table tr{border-bottom:1px solid rgba(255,255,255,.05);transition:background .15s}
    .env-table tr:last-child{border-bottom:none}
    .env-table tr:hover{background:rgba(255,255,255,.03)}
    .env-table td{padding:10px 20px;font-family:'JetBrains Mono',monospace;font-size:12px;vertical-align:top}
    .env-key{color:#94a3b8;width:220px;font-weight:500;white-space:nowrap}
    .env-val{color:#e2e8f0;word-break:break-all}

    /* ── post body ── */
    .post-body{padding:16px 20px;font-family:'JetBrains Mono',monospace;font-size:12px;color:#4ade80;line-height:1.8;white-space:pre-wrap;word-break:break-all}

    /* ── footer ── */
    footer{position:relative;z-index:1;border-top:1px solid rgba(255,255,255,.09);padding:32px 40px;margin-top:48px}
    .foot-inner{max-width:900px;margin:0 auto;display:flex;justify-content:space-between;align-items:center;gap:20px}
    .foot-logo{font-family:'Syne',sans-serif;font-weight:800;font-size:16px;background:linear-gradient(130deg,#00f0d4,#a78bfa);-webkit-background-clip:text;-webkit-text-fill-color:transparent;background-clip:text}
    .foot-copy{font-size:13px;color:#475569}
    .foot-links{display:flex;gap:20px;list-style:none}
    .foot-links a{font-size:13px;color:#94a3b8;text-decoration:none;transition:color .2s}
    .foot-links a:hover{color:#fff}

    @keyframes fadeU{from{opacity:0;transform:translateY(10px)}to{opacity:1;transform:translateY(0)}}
    .page{animation:fadeU .5s ease both}
  </style>
</head>
<body>
  <div class="bg"></div>

  <nav>
    <a class="nav-logo" href="/">webserv</a>
    <ul class="nav-links">
      <li><a href="/">Home</a></li>
      <li><a href="/upload.html">Upload</a></li>
      <li><a href="/index2.html">Playground</a></li>
    </ul>
    <div class="nav-pill"><div class="pulse"></div>CGI · PHP</div>
  </nav>

  <div class="page">
    <div class="label">CGI · PHP Test</div>
    <h1>PHP Environment</h1>
    <p class="sub"><?php echo $_SERVER['SCRIPT_FILENAME']; ?></p>

    <!-- quick chips -->
    <div class="chips">
      <div class="chip">
        <div class="chip-dot" style="background:#00f0d4;box-shadow:0 0 6px #00f0d4"></div>
        <span class="chip-key">Method</span>
        <span class="chip-val"><?php echo htmlspecialchars($_SERVER['REQUEST_METHOD']); ?></span>
      </div>
      <div class="chip">
        <div class="chip-dot" style="background:#a78bfa;box-shadow:0 0 6px #a78bfa"></div>
        <span class="chip-key">PHP</span>
        <span class="chip-val"><?php echo phpversion(); ?></span>
      </div>
      <?php if (!empty($_SERVER['QUERY_STRING'])): ?>
      <div class="chip">
        <div class="chip-dot" style="background:#60a5fa;box-shadow:0 0 6px #60a5fa"></div>
        <span class="chip-key">Query</span>
        <span class="chip-val"><?php echo htmlspecialchars($_SERVER['QUERY_STRING']); ?></span>
      </div>
      <?php endif; ?>
      <div class="chip">
        <div class="chip-dot" style="background:#fb7185;box-shadow:0 0 6px #fb7185"></div>
        <span class="chip-key">Vars</span>
        <span class="chip-val"><?php echo count($_SERVER); ?> entries</span>
      </div>
    </div>

    <?php if ($_SERVER['REQUEST_METHOD'] === 'POST'): ?>
    <!-- POST body -->
    <div class="card">
      <div class="card-head">
        <div class="card-head-icon" style="background:rgba(74,222,128,.1);border:1px solid rgba(74,222,128,.2)">
          <svg viewBox="0 0 24 24" style="stroke:#4ade80">
            <polyline points="16 18 22 12 16 6"/><polyline points="8 6 2 12 8 18"/>
          </svg>
        </div>
        <span class="card-title" style="color:#4ade80">POST Body</span>
      </div>
      <div class="post-body"><?php echo htmlspecialchars(file_get_contents("php://stdin")); ?></div>
    </div>
    <?php endif; ?>

    <!-- env table -->
    <div class="card">
      <div class="card-head">
        <div class="card-head-icon" style="background:rgba(167,139,250,.1);border:1px solid rgba(167,139,250,.2)">
          <svg viewBox="0 0 24 24" style="stroke:#a78bfa">
            <rect x="2" y="3" width="20" height="14" rx="2"/>
            <line x1="8" y1="21" x2="16" y2="21"/>
            <line x1="12" y1="17" x2="12" y2="21"/>
          </svg>
        </div>
        <span class="card-title" style="color:#a78bfa">$_SERVER</span>
        <span class="card-count"><?php echo count($_SERVER); ?> entries</span>
      </div>
      <table class="env-table">
        <?php foreach ($_SERVER as $key => $val): ?>
        <tr>
          <td class="env-key"><?php echo htmlspecialchars($key); ?></td>
          <td class="env-val"><?php echo htmlspecialchars((string)$val); ?></td>
        </tr>
        <?php endforeach; ?>
      </table>
    </div>
  </div>

  <footer>
    <div class="foot-inner">
      <div class="foot-logo">webserv</div>
      <span class="foot-copy">© 2025 REDA, ELAMRANI, SIMO · 42 School</span>
      <ul class="foot-links">
        <li><a href="/">Home</a></li>
        <li><a href="/upload.html">Upload</a></li>
        <li><a href="/404.html">404</a></li>
        <li><a href="/500.html">500</a></li>
      </ul>
    </div>
  </footer>
</body>
</html>