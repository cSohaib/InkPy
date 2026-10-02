# InkPy

Text and mathematics on the same page.

## A little algebra

The relation $a^2+b^2=c^2$ fits inside a sentence. A fraction $\frac{1}{2}$ and a root $\sqrt{x^2+y^2}$ can sit beside ordinary text.

The quadratic formula is:

$$
x=\frac{-b\pm\sqrt{b^2-4ac}}{2a}
$$

## Matrices and sums

A small matrix:

$$
A=\begin{pmatrix}1 & 2 \\ 3 & 4\end{pmatrix}
$$

A sum with a limit:

$$
\lim_{n\to\infty}\frac{1}{n}\sum_{i=1}^{n} f(i/n)
=\int_0^1 f(x)\,dx
$$

Inline code remains literal: `$x^2$`.

## Readable fallback

An unsupported formula stays visible as source:

$$
\inkpyUnknownCommand{x}
$$
