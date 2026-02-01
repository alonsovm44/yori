### Semantic Diff Report

**1. Brief Summary of Changes:**

The primary change is a complete transformation from a simple **square outline** in `shape.svg` to a **graph visualization** in `graph.svg`. This involves the introduction of **nodes**, **edges**, and **labels**.

**2. Changed Functions (What changed and where):**

*   **`shape.svg`:** Contains four `<line>` elements defining the perimeter of a square.
*   **`graph.svg`:**
    *   **Edges:** Introduces **five `<line>` elements** representing connections between nodes. The `stroke` color and `stroke-width` attributes are also modified.
    *   **Nodes:** Adds **five `<circle>` elements** to represent data points or entities within the graph. Attributes like `cx`, `cy`, `r`, `fill`, and `stroke` are used.
    *   **Labels:** Incorporates **five `<text>` elements** to display numerical labels associated with the nodes. Attributes like `x`, `y`, `font-size`, and `text-anchor` are utilized.
    *   **`viewBox` Attribute:** The `viewBox` attribute has been significantly altered from `0 0 1 1` to `-5 0 10 6` to accommodate the new graph elements.

**3. Additional Observations:**

*   The original square drawing in `shape.svg` is entirely replaced, not modified.
*   The `graph.svg` file represents a **directed graph** due to the nature of the line elements connecting specific points.
*   The labels appear to be numerical values, suggesting a data-driven visualization.
*   The `width` and `height` attributes in `shape.svg` are fixed, whereas `graph.svg` uses `width="100%"` and `height="100%"`, making it **responsive**.

Would you like to delve deeper into the specific attributes of the nodes or edges?