import streamlit as st
import pandas as pd
import altair as alt
from streamlit_autorefresh import st_autorefresh

st.set_page_config(layout="wide")
st.title("📈 Order Matching Engine Dashboard")

# Refresh every 5 seconds
st_autorefresh(interval=500, key="auto-refresh")

# Load data
trades = pd.read_csv("trades.csv")
buy_orders = pd.read_csv("buy_orders.csv")
sell_orders = pd.read_csv("sell_orders.csv")
order_status = pd.read_csv("order_status.csv")

# --- Live Market Ticker ---
if not trades.empty:
    latest_trade = trades.iloc[-1]
    st.markdown(f"### 💹 Live Ticker: ₹{latest_trade['Price']} | Qty: {latest_trade['Quantity']} | Time: {latest_trade['Timestamp']}")
else:
    st.markdown("### 💹 Live Ticker: No trades yet.")

# --- Metrics Summary ---
st.subheader("📊 Market Summary")
col1, col2 = st.columns(2)

if not trades.empty:
    latest_price = trades.iloc[-1]['Price']
    total_volume = trades['Quantity'].sum()
    col1.metric("Last Traded Price", f"₹{latest_price}")
    if isinstance(total_volume, (int, float)):
        formatted_volume = f"{total_volume/1_000_000:.1f}M" if total_volume >= 1_000_000 else f"{total_volume:,}"
    else:
        formatted_volume = total_volume
    col2.metric("Total Volume Traded", f"{formatted_volume} units")
else:
    col1.warning("No trades yet.")
    col2.warning("No volume recorded.")

# --- Price Movements Line Chart ---
st.subheader("📈 Price Trend Over Time")
if not trades.empty:
    price_chart = alt.Chart(trades).mark_line(point=True).encode(
        x=alt.X("Timestamp:Q", title="Time"),
        y=alt.Y("Price:Q", title="Price")
    ).properties(height=300)
    st.altair_chart(price_chart, use_container_width=True)
else:
    st.info("No trade data to display.")

# --- Depth Charts ---
st.subheader("📉 Market Depth")
col1, col2 = st.columns(2)

with col1:
    st.markdown("### 🟦 Buy Orders")
    if not buy_orders.empty:
        buy_depth = buy_orders.groupby("Price")["Quantity"].sum().reset_index()
        chart = alt.Chart(buy_depth).mark_bar(color='green').encode(
            x=alt.X("Price:O", sort='descending'),
            y="Quantity:Q"
        )
        st.altair_chart(chart, use_container_width=True)
    else:
        st.info("No active buy orders.")

with col2:
    st.markdown("### 🟥 Sell Orders")
    if not sell_orders.empty:
        sell_depth = sell_orders.groupby("Price")["Quantity"].sum().reset_index()
        chart = alt.Chart(sell_depth).mark_bar(color='crimson').encode(
            x=alt.X("Price:O", sort='ascending'),
            y="Quantity:Q"
        )
        st.altair_chart(chart, use_container_width=True)
    else:
        st.info("No active sell orders.")

# --- Active Orders Table ---
st.subheader("📦 Active Orders")
col3, col4 = st.columns(2)

with col3:
    st.markdown("#### 🔵 Active Buy Orders")
    st.dataframe(buy_orders[::-1], use_container_width=True)

with col4:
    st.markdown("#### 🔴 Active Sell Orders")
    st.dataframe(sell_orders[::-1], use_container_width=True)

# --- Order Status Pie Chart ---
st.subheader("📌 Order Status Distribution")
if not order_status.empty:
    status_data = order_status["Status"].value_counts().reset_index()
    status_data.columns = ["Status", "Count"]
    pie_chart = alt.Chart(status_data).mark_arc().encode(
        theta="Count",
        color="Status",
        tooltip=["Status", "Count"]
    )
    st.altair_chart(pie_chart, use_container_width=True)
else:
    st.info("No order status data available.")

# --- All Trades Table ---
st.subheader("📄 Trade Log")
if not trades.empty:
    grouped = trades.groupby("Price")["Quantity"].sum().reset_index().sort_values("Price")
    st.markdown("#### 🔁 Grouped Trades (Price vs Total Quantity)")
    st.bar_chart(grouped.set_index("Price"))

    st.markdown("#### 📋 Recent Trade History")
    st.dataframe(trades[::-1], use_container_width=True)
else:
    st.info("No trades yet.")
