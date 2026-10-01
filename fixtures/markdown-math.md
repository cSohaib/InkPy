# InkPy Markdown and math acceptance corpus

Proposed coverage, not implemented support. Use the same file for desktop and device comparisons. Check portrait/landscape, night mode and selectable text fonts; compare meaning, baseline and clipping rather than identical browser pixels.

## Text and structure

A paragraph with **bold**, *italic*, ~~struck text~~ and `inline_code()`. Accented text: café, français, Lëtzebuerg, Straße. Preserve Unicode; unsupported glyphs need a visible fallback.

> A quotation spanning more than one line should wrap within its available width.

1. First ordered item.
2. Second item with nested content:
   - First nested item.
   - Second nested item.

---

| Symbol | Meaning |
|---|---|
| $x_i$ | Indexed variable |
| $\alpha$ | Greek letter |
| $\mathbb{R}$ | Real numbers |

A [reference link][example] has a definition later in the document.
An [inline link](https://example.com) is readable text; no browser/network feature is requested.
![Local image acceptance placeholder](assets/sample.png)

The image above is deliberately missing in this planning commit. Expect a readable missing-image fallback. Add one small real local PNG during the rendering prototype to test scaling without requiring a large decode buffer.

## Literal dollar handling

Escaped currency: \$5.00.
Inline code must remain literal: `$x^2$`.
Unclosed math delimiter remains visible: $unfinished

```python
print('$not_math$')
# $$ is literal inside fenced code
```

## Inline math

The relation $a^2+b^2=c^2$ must align with this sentence.

Indices and bounds: $x_{i,j}^{(k)}$, $\sum_{i=1}^{n} x_i$, $\int_0^1 x^2\,dx$.

Symbols: $\alpha+\beta\leq\gamma$, $\forall x\in\mathbb{R}$, $\nabla f(x)$, $\infty$, $\partial f/\partial x$.

Nested size: $\frac{1}{1+\frac{1}{x}}$, $\sqrt{x^2+y^2}$, $\sqrt[3]{8}$.

Text and styles: $\text{subject to}$, $\mathrm{kg}$, $\mathbf{x}$, $\mathcal{L}$, $\mathbb{R}$.

## Display math

$$
x=\frac{-b\pm\sqrt{b^2-4ac}}{2a}
$$

$$
\left(\frac{x+1}{x-1}\right)^2
+\left\lVert \mathbf{x}\right\rVert_2
$$

$$
A=\begin{pmatrix}
1 & 2 \\
3 & 4
\end{pmatrix}
$$

$$
\begin{aligned}
\min_{\mathbf{x}}\quad & \mathbf{c}^{T}\mathbf{x}\\
\text{subject to}\quad & A\mathbf{x}\leq\mathbf{b}\\
& \mathbf{x}\geq 0
\end{aligned}
$$

$$
f(x)=\begin{cases}
x^2 & x\geq 0\\
-x & x<0
\end{cases}
$$

$$
\lim_{n\to\infty}\frac{1}{n}\sum_{i=1}^{n} f(i/n)
=\int_0^1 f(x)\,dx
$$

## Explicit fallback cases

Unknown command (show source/error without crashing):

$$
\inkpyUnknownCommand{x}
$$

Malformed grouping:

$$
\frac{1}{x
$$

Over-wide expression (verify the chosen no-swipe overflow policy):

$$
a_1+a_2+a_3+a_4+a_5+a_6+a_7+a_8+a_9+a_{10}+a_{11}+a_{12}+a_{13}+a_{14}+a_{15}
$$

[example]: https://example.com

## Future generated stress fixtures

Generate during the relevant prototype, not as large committed blobs:
- Many paragraphs and headings beyond available RAM; index/page without full-file loading.
- A very long single line, paragraph, code block and table cell.
- Code fence/list/reference crossing a parser chunk boundary.
- UTF-8 sequences and CRLF crossing read boundaries; optional UTF-8 BOM.
- Binary NUL and invalid encoding after a text-looking prefix.
- Deeply nested or huge math with an explicit complexity error and intact navigation.
- Python output flood; stop request during busy loop and blocking network/file operation.
- Dictionary index and definition larger than the RAM cache.
