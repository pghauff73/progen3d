(function () {
  "use strict";

  document.querySelectorAll("object[data-learning-diagram]").forEach(function (diagramObject) {
    diagramObject.addEventListener("load", function () {
      let diagramDocument;
      try {
        diagramDocument = diagramObject.contentDocument;
      } catch (error) {
        return;
      }
      if (!diagramDocument) {
        return;
      }

      const liveRegion = document.querySelector("[data-diagram-live-region]");
      diagramDocument.querySelectorAll("[tabindex='0']").forEach(function (diagramNode) {
        diagramNode.addEventListener("focus", function () {
          if (liveRegion) {
            liveRegion.textContent = diagramNode.getAttribute("aria-label") || "Diagram node focused";
          }
        });
      });
    });
  });
})();
