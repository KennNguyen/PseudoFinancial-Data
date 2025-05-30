from fastapi import FastAPI, Query, HTTPException, Request
from fastapi.responses import JSONResponse, RedirectResponse
from fastapi.staticfiles import StaticFiles
from fastapi.middleware.cors import CORSMiddleware

import subprocess
import pandas as pd
import numpy as np
import os
import sys

from slowapi import Limiter, _rate_limit_exceeded_handler
from slowapi.util import get_remote_address
from slowapi.errors import RateLimitExceeded

# ------------------ Initialize FastAPI with SlowAPI Limiter ------------------
limiter = Limiter(key_func=get_remote_address, default_limits=["20/minute"])

app = FastAPI()
app.state.limiter = limiter
app.add_exception_handler(RateLimitExceeded, _rate_limit_exceeded_handler)

# ------------------ CORS ------------------
app.add_middleware(
    CORSMiddleware,
    allow_origins=["https://kennnguyendev.com", "https://www.kennnguyendev.com"],
    allow_credentials=True,
    allow_methods=["GET"],
    allow_headers=["Content-Type"],
)

# ------------------ Static ------------------
app.mount("/static", StaticFiles(directory="static", html=True), name="static")

@app.get("/")
async def root():
    return RedirectResponse(url="/static/index.html")

# ------------------ Binary Paths ------------------
is_windows = sys.platform.startswith("win")
current_dir = os.path.abspath(os.path.dirname(__file__))

factor_binary = os.path.join(current_dir, "factor_model.exe" if is_windows else "factor_model")
heston_binary = os.path.join(current_dir, "heston_model.exe" if is_windows else "heston_model")

# ------------------ Simulate Endpoint ------------------
@app.get("/simulate")
@limiter.limit("10/minute")
async def simulate(
    request: Request,
    duration: int = Query(100, ge=1, le=10000),
    volatility: float = Query(0.2, ge=0.0, le=5.0),
    seed: int = Query(42),
    num_assets: int = Query(1, ge=1, le=50),
    initial_price: float = Query(100.0, gt=0),
    initial_variance: float = Query(0.04, ge=0),
    kappa: float = Query(2.0, ge=0),
    theta: float = Query(0.04, ge=0),
    sigma_v: float = Query(0.3, ge=0),
    rho: float = Query(-0.7, ge=-1.0, le=1.0),
    dt: float = Query(0.01, gt=0),
    idiosyncratic: float = Query(0.1, ge=0),
    factor_exposures: str = Query("1")
):
    if not os.path.isfile(factor_binary):
        raise HTTPException(status_code=500, detail="Factor model binary not found.")
    if not os.path.isfile(heston_binary):
        raise HTTPException(status_code=500, detail="Heston model binary not found.")

    try:
        subprocess.run(
            [factor_binary, str(duration), str(volatility), str(num_assets), str(seed)],
            check=True, capture_output=True, text=True
        )
    except subprocess.CalledProcessError as e:
        raise HTTPException(status_code=500, detail=f"Factor model error: {e.stderr}")

    if not os.path.exists("factor_output.csv"):
        raise HTTPException(status_code=500, detail="Missing factor_output.csv")

    factor_df = pd.read_csv("factor_output.csv")
    factor_levels = factor_df.iloc[:, -1].tolist()

    try:
        exposures = [float(x.strip()) for x in factor_exposures.split(',')]
    except ValueError:
        raise HTTPException(status_code=400, detail="Invalid format for factor_exposures.")

    exposure_args = [str(e) for e in exposures]

    try:
        subprocess.run(
            [
                heston_binary,
                str(initial_price), str(initial_variance),
                str(kappa), str(theta), str(sigma_v), str(rho), str(dt),
                str(idiosyncratic), str(duration), *exposure_args
            ],
            check=True, capture_output=True, text=True
        )
    except subprocess.CalledProcessError as e:
        raise HTTPException(status_code=500, detail=f"Heston model error: {e.stderr}")

    if not os.path.exists("heston_output.csv"):
        raise HTTPException(status_code=500, detail="Missing heston_output.csv")

    heston_df = pd.read_csv("heston_output.csv")
    heston_prices = heston_df["price"].tolist()
    heston_variances = heston_df["variance"].tolist()

    for f in ["factor_output.csv", "heston_output.csv"]:
        try:
            os.remove(f)
        except Exception:
            pass

    return JSONResponse(content={
        "factor_levels": factor_levels,
        "heston_prices": heston_prices,
        "heston_variances": heston_variances
    })
