# InkPy reader

A small page, with **bold text**, *emphasis* and `inline code`.

## First chapter

This prototype keeps page positions on disk. Side buttons will turn pages; the Home menu will show the current page and chapter.

- Plain filenames
- No thumbnails
- No swipe gestures

### A smaller heading

Only second-level headings become chapters. A café, Greek α and β, and numeric entities like &#960; retain their UTF-8 text.

```python
# code, not a chapter
print("Hello, InkPy")
```

## Mathematics later

Math source is retained here: $x^2 + y^2 = z^2$. Connecting the existing math renderer to this layout is a separate stage.

Second chapter style
--------------------

Setext second-level headings also become chapter entries. Font changes and orientation require rebuilding page positions, while source offsets help restore the reading position.

| Item | State |
| --- | --- |
| Text | Prototype |
| Device | Pending |
