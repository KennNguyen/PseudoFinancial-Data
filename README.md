# PseudoFinancial-Data

## Overview

**PseudoFinancial-Data** is a real-time simulation platform for generating, distributing, and analyzing synthetic financial market data using modern systems tools and realistic stochastic models. It is designed for quantitative finance research, strategy prototyping, market behavior analysis, and systems engineering training.

Built with performance in mind, the system integrates high-speed C++ simulations (factor and Heston models), FastAPI endpoints, real-time streaming via Kafka, in-memory caching via Redis, and scalable storage via PostgreSQL + TimescaleDB. A responsive frontend powered by S3 + CloudFront visualizes the output for inspection and educational purposes.

---

## Architecture Highlights

| Layer                  | Stack                                                                 |
|------------------------|-----------------------------------------------------------------------|
| **Simulation Engine**  | C++ (custom factor and Heston models)                                 |
| **Backend API**        | FastAPI (Python)                                                      |
| **Real-Time Streaming**| Kafka                                                                 |
| **Low-Latency Cache**  | Redis                                                                 |
| **Persistent Storage** | PostgreSQL + TimescaleDB                                              |
| **Visualization**      | Chart.js, MathJax, HTML/CSS hosted via S3 + CloudFront                |
| **CI/CD Deployment**   | GitHub Actions → S3 (via `aws s3 sync`)                               |

---

## ✅ Stage One: C++ Simulation Core (Completed)

- **Factor Model (`factor_model.cpp`)**:  
  Simulates asset returns using a Gaussian single-factor model with asset-specific beta exposures and idiosyncratic shocks. Factor increments follow a random walk distribution.

- **Heston Model (`heston_model.cpp`)**:  
  Simulates asset prices using the Heston stochastic volatility model with correlation to the factor model's systemic shocks. Euler-Maruyama discretization is used, with variance clamping to ensure numerical stability.

---

## 🚀 Stage Two: Data Distribution + Real-Time Processing (Ongoing)

- **Kafka**:  
  Acts as the primary event stream pipeline, distributing tick-level data across consumers.

- **Redis**:  
  Caches the latest market data in memory for low-latency reads. Allows real-time web dashboards and backtesters to query the most recent data instantly.

---

## 🧠 Stage Three: Historical Storage (Ongoing)

- **PostgreSQL** (with **TimescaleDB**):  
  Stores long-term historical market simulations. Schema supports efficient querying via hypertables and time-based partitioning. Indexing strategies are under evaluation for large-scale multi-asset data retention.

---

## 🔬 Research-Oriented Modeling Approach

- **Data Generation**:
  - Simulate price and volatility using Heston dynamics with both systemic and idiosyncratic risk sources
  - Generate correlated factor returns for portfolio-wide behavior modeling
  - Use numerically stable and reproducible simulation methods in C++

- **Streaming + Caching**:
  - Publish tick-level price data using Kafka producers
  - Cache “hot” symbols in Redis to simulate real-world data prioritization

- **Storage Design**:
  - Store cold data in TimescaleDB and CSV
  - Query via hypertables for efficient time-range slicing

- **Deployment**:
  - Static frontend hosted via **AWS S3 + CloudFront**
  - GitHub Actions auto-sync public files to S3 while excluding sensitive files like `.yml`
  - Bucket is secured with **OAC (Origin Access Control)** instead of public access

---

## 🛠️ Technologies Used

- **Languages**:
  - `C++` — High-performance simulation engine
  - `Python` — FastAPI backend and control logic
  - `SQL` — Time-series schema + queries (TimescaleDB)

- **Simulation & Scientific Libraries**:
  - `Eigen` — Matrix computation in C++
  - `NumPy`, `pandas`, `SciPy` — Python-side analysis

- **Streaming & Caching**:
  - `Apache Kafka` — Real-time pub/sub
  - `Redis` — Ultra-low-latency cache

- **Storage**:
  - `PostgreSQL + TimescaleDB` — Persistent historical storage
  - `CSV` — Export and debugging

- **Frontend**:
  - `Chart.js` — Dynamic visualizations
  - `MathJax` — Math documentation rendering
  - `HTML/CSS` — Custom layout
  - `AWS S3 + CloudFront` — Secure and scalable hosting

- **DevOps**:
  - `Docker` — Containerization
  - `GitHub Actions` — CI/CD pipeline for syncing static assets to S3

---

## 📈 Future Enhancements

- Add jump-diffusion processes and rough volatility dynamics (e.g., Rough Heston)
- Extend the factor model to multi-factor, correlated market drivers
- Implement REST streaming endpoints (e.g., Server-Sent Events or WebSockets)
- Add analytics tools for tracking strategy PnL or drawdowns on simulated data
- Embed filters (e.g., Kalman) for latent state inference

---
