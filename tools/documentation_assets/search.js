(function () {
  "use strict";

  const searchInput = document.querySelector("[data-documentation-search]");
  const searchResults = Array.from(document.querySelectorAll("[data-search-record]"));
  const resultSummary = document.querySelector("[data-search-summary]");

  if (!searchInput || searchResults.length === 0) {
    return;
  }

  function normalized(value) {
    return value.toLocaleLowerCase().normalize("NFKD");
  }

  function updateResults() {
    const query = normalized(searchInput.value.trim());
    let visibleCount = 0;

    searchResults.forEach(function (result) {
      const searchText = normalized(result.dataset.searchRecord || "");
      const isVisible = query.length === 0 || searchText.includes(query);
      result.hidden = !isVisible;
      if (isVisible) {
        visibleCount += 1;
      }
    });

    if (resultSummary) {
      resultSummary.textContent = visibleCount + " documentation page" + (visibleCount === 1 ? "" : "s") + " shown";
    }
  }

  searchInput.addEventListener("input", updateResults);
  updateResults();
})();
