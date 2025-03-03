import marimo

__generated_with = "0.11.7"
app = marimo.App(
    width="medium",
    app_title="Gemstone Examples",
    css_file="",
    html_head_file="",
)


@app.cell(hide_code=True)
def _(mo):
    mo.md(r"""**Gemstone Examples**""")
    return


@app.cell
def _():
    import marimo as mo
    return (mo,)


@app.cell
def _():
    import numpy as np
    return (np,)


@app.cell
def _():
    import seaborn as sns
    return (sns,)


@app.cell
def _():
    import matplotlib.pyplot as plt
    return (plt,)


@app.cell
def _(np, plt, sns):
    sns.set_theme(style="dark")

    # Simulate data from a bivariate Gaussian
    n = 10000
    mean = [0, 0]
    cov = [(2, .4), (.4, .2)]
    rng = np.random.RandomState(0)
    x, y = rng.multivariate_normal(mean, cov, n).T

    # Draw a combo histogram and scatterplot with density contours
    f, ax = plt.subplots(figsize=(6, 6))
    sns.scatterplot(x=x, y=y, s=5, color=".15")
    sns.histplot(x=x, y=y, bins=50, pthresh=.1, cmap="mako")
    sns.kdeplot(x=x, y=y, levels=5, color="w", linewidths=1)
    return ax, cov, f, mean, n, rng, x, y


@app.cell
def _(sns):
    sns.set_theme(style="ticks")

    df = sns.load_dataset("penguins")
    sns.pairplot(df, hue="species")
    return (df,)


@app.cell
def _():
    import polars as pl
    import datetime as dt
    return dt, pl


@app.cell
def _(dt, pl):
    df2 = pl.DataFrame(
        {
            "name": ["Alice Archer", "Ben Brown", "Chloe Cooper", "Daniel Donovan"],
            "birthdate": [
                dt.date(1997, 1, 10),
                dt.date(1985, 2, 15),
                dt.date(1983, 3, 22),
                dt.date(1981, 4, 30),
            ],
            "weight": [57.9, 72.5, 53.6, 83.1],  # (kg)
            "height": [1.56, 1.77, 1.65, 1.75],  # (m)
        }
    )

    print(df2)
    return (df2,)


if __name__ == "__main__":
    app.run()
