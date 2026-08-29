(function () {
  "use strict";

  const navigationToggle = document.querySelector("[data-navigation-toggle]");
  const siteNavigation = document.querySelector("[data-site-navigation]");

  if (navigationToggle && siteNavigation) {
    navigationToggle.addEventListener("click", function () {
      const isOpen = siteNavigation.dataset.open === "true";
      siteNavigation.dataset.open = String(!isOpen);
      navigationToggle.setAttribute("aria-expanded", String(!isOpen));
    });
  }

  document.querySelectorAll("pre").forEach(function (codeBlock, index) {
    const copyButton = document.createElement("button");
    copyButton.type = "button";
    copyButton.className = "copy-code-button";
    copyButton.textContent = "Copy code";
    copyButton.setAttribute("aria-label", "Copy code example " + (index + 1));
    codeBlock.before(copyButton);

    copyButton.addEventListener("click", async function () {
      try {
        await navigator.clipboard.writeText(codeBlock.innerText);
        copyButton.textContent = "Copied";
      } catch (error) {
        copyButton.textContent = "Copy unavailable";
      }
      window.setTimeout(function () {
        copyButton.textContent = "Copy code";
      }, 1800);
    });
  });

  const progressButton = document.querySelector("[data-progress-key]");
  const progressStatus = document.querySelector("[data-progress-status]");

  if (progressButton && progressStatus) {
    const storageKey = "progen3d-doc-progress:" + progressButton.dataset.progressKey;

    function renderProgress(isComplete) {
      progressButton.setAttribute("aria-pressed", String(isComplete));
      progressButton.textContent = isComplete ? "Mark as not complete" : "Mark lesson complete";
      progressStatus.textContent = isComplete ? "Completed on this browser" : "Not yet completed";
    }

    let isComplete = false;
    try {
      isComplete = window.localStorage.getItem(storageKey) === "complete";
    } catch (error) {
      progressStatus.textContent = "Local progress storage is unavailable";
    }
    renderProgress(isComplete);

    progressButton.addEventListener("click", function () {
      isComplete = progressButton.getAttribute("aria-pressed") !== "true";
      try {
        if (isComplete) {
          window.localStorage.setItem(storageKey, "complete");
        } else {
          window.localStorage.removeItem(storageKey);
        }
      } catch (error) {
        progressStatus.textContent = "Local progress storage is unavailable";
        return;
      }
      renderProgress(isComplete);
    });
  }
})();
