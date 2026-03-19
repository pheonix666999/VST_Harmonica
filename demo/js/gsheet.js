/**
 * Google Sheets loader for public sheets.
 * Uses the GViz endpoint, so the sheet must be shared publicly.
 */

class GSheetPresets {
    static buildUrl(sheetId, sheetName) {
        const safeSheet = encodeURIComponent(sheetName || 'Presets');
        return `https://docs.google.com/spreadsheets/d/${sheetId}/gviz/tq?tqx=out:json&sheet=${safeSheet}`;
    }

    static parseResponse(text) {
        const start = text.indexOf('{');
        const end = text.lastIndexOf('}');
        if (start < 0 || end < 0 || end <= start) {
            throw new Error('Invalid Google Sheets response');
        }
        return JSON.parse(text.slice(start, end + 1));
    }

    static cellValue(cell) {
        if (!cell) return '';
        if (typeof cell.v === 'boolean') return cell.v;
        if (typeof cell.v === 'number') return cell.v;
        if (typeof cell.f === 'string' && cell.f.trim() !== '') return cell.f.trim();
        return cell.v ?? '';
    }

    static normalizeHeader(header) {
        return String(header || '')
            .trim()
            .toLowerCase()
            .replace(/\s+/g, '_');
    }

    static async loadRows({ sheetId, sheetName }) {
        if (!sheetId) {
            throw new Error('Missing Google Sheet ID');
        }

        const url = this.buildUrl(sheetId, sheetName);
        const response = await fetch(url);
        if (!response.ok) {
            throw new Error(`Google Sheets request failed (${response.status})`);
        }

        const text = await response.text();
        const parsed = this.parseResponse(text);
        const table = parsed.table;
        if (!table || !Array.isArray(table.cols) || !Array.isArray(table.rows)) {
            throw new Error('Sheet has no table data');
        }

        const headers = table.cols.map((col, idx) => {
            const label = this.normalizeHeader(col.label);
            return label || `col_${idx + 1}`;
        });

        return table.rows
            .map(row => {
                const out = {};
                headers.forEach((header, idx) => {
                    out[header] = this.cellValue(row.c?.[idx]);
                });
                return out;
            })
            .filter(row => Object.values(row).some(v => String(v).trim() !== ''));
    }
}
