from fastapi import FastAPI, Query
from fastapi.responses import JSONResponse
from fastapi.staticfiles import StaticFiles
import subprocess
import pandas as pd
import numpy as np
import os
import sys

app = FastAPI()

# Serve static files (for index.html and others)
app.mount("/static", StaticFiles(directory="static"), name="static")

# Redirect root "/" to the index.html page
@app.get("/")
async def root():
    from fastapi.responses import RedirectResponse
    return RedirectResponse(url="/static/index.html")

# ---------------------------- PATH FIX -------------------------------

# Detect if running on Windows or Linux
is_windows = sys.platform.startswith('win')

# Get the absolute path where main.py is located (should be /usr/src/app/app/web)
current_dir = os.path.dirname(__file__)

# Build absolute paths to the binaries so subprocess always finds them
factor_binary = os.path.join(current_dir, "factor_model.exe" if is_windows else "factor_model")
heston_binary = os.path.join(current_dir, "heston_model.exe" if is_windows else "heston_model")

# ---------------------------- SIMULATION ENDPOINT --------------------

@app.get("/simulate")
async def simulate(
    duration: int = Query(100),
    volatility: float = Query(0.2),
    seed: int = Query(42),
    num_assets: int = Query(1),
    initial_price: float = Query(100),
    initial_variance: float = Query(0.04),
    kappa: float = Query(2.0),
    theta: float = Query(0.04),
    sigma_v: float = Query(0.3),
    rho: float = Query(-0.7),
    dt: float = Query(0.01),
    idiosyncratic: float = Query(0.1),
    factor_exposures: str = Query("1")
):

    # ----------------- RUN FACTOR MODEL -----------------

    subprocess.run([
        factor_binary,
        str(duration),
        str(volatility),
        str(num_assets),
        str(seed)
    ], check=True)

    factor_df = pd.read_csv("factor_output.csv")
    factor_levels = factor_df.iloc[:, -1].tolist()

    # ----------------- PREPARE EXPOSURES -----------------

    exposures = [float(x) for x in factor_exposures.split(',')]
    exposure_args = [str(e) for e in exposures]

    # ----------------- RUN HESTON MODEL -----------------

    subprocess.run([
        heston_binary,
        str(initial_price),
        str(initial_variance),
        str(kappa),
        str(theta),
        str(sigma_v),
        str(rho),
        str(dt),
        str(idiosyncratic),
        str(duration),
        *exposure_args
    ], check=True)

    heston_df = pd.read_csv("heston_output.csv")
    heston_prices = heston_df["price"].tolist()
    heston_variances = heston_df["variance"].tolist()

    # ----------------- CLEAN UP CSV FILES -----------------

    for file in ["factor_output.csv", "heston_output.csv"]:
        try: os.remove(file)
        except FileNotFoundError: pass


    # ----------------- RETURN RESULTS -----------------

    return JSONResponse(content={
        "factor_levels": factor_levels,
        "heston_prices": heston_prices,
        "heston_variances": heston_variances
    })
