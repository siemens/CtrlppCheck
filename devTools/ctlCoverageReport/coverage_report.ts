#!/usr/bin/env node
/**
 * WinCC OA Coverage Report Generator
 * Merges multiple coverage XML files and generates a command-line report.
 * 
 * Usage: npx ts-node coverage_report.ts [path] [-f filter] [-v] [-o output.xml]
 *    or: node coverage_report.js [path] [-f filter] [-v] [-o output.xml]
 */

import * as fs from 'fs';
import * as path from 'path';

interface FileData {
  lines: Map<number, number>;
  functions: Map<string, number>;
  branches: { total: number; executed: number };
}

interface CoverageData {
  files: Map<string, FileData>;
}

interface FileStat {
  path: string;
  lines: number;
  covered: number;
  lineRate: number;
  functions: number;
  funcCovered: number;
  funcRate: number;
}

function createFileData(): FileData {
  return {
    lines: new Map(),
    functions: new Map(),
    branches: { total: 0, executed: 0 }
  };
}

function parseCoverageXml(xmlContent: string, coverageData: CoverageData, filterPath?: string): void {
  // Simple XML parsing using regex (avoiding external dependencies)
  const scriptRegex = /<script>([\s\S]*?)<\/script>/g;
  let scriptMatch;

  while ((scriptMatch = scriptRegex.exec(xmlContent)) !== null) {
    const scriptContent = scriptMatch[1];

    // Extract file path
    const filePathMatch = /<file path="([^"]+)"/.exec(scriptContent);
    if (!filePathMatch) continue;

    let filepath = filePathMatch[1].replace(/\\/g, '/');
    filepath = path.normalize(filepath);

    // Apply filter
    if (filterPath && !filepath.toLowerCase().includes(filterPath.toLowerCase())) {
      continue;
    }

    if (!coverageData.files.has(filepath)) {
      coverageData.files.set(filepath, createFileData());
    }
    const fileData = coverageData.files.get(filepath)!;

    // Parse functions
    const funcRegex = /<func\s+line="(\d+)"\s*\n?\s*name="([^"]+)"\s*\n?\s*signature="([^"]*)"\s*\n?\s*count="(\d+)"[^>]*>([\s\S]*?)<\/func>/g;
    let funcMatch;

    while ((funcMatch = funcRegex.exec(scriptContent)) !== null) {
      const funcName = funcMatch[2];
      const funcCount = parseInt(funcMatch[4], 10);
      const funcBody = funcMatch[5];

      // Update function count (max)
      const existingCount = fileData.functions.get(funcName) || 0;
      fileData.functions.set(funcName, Math.max(existingCount, funcCount));

      // Parse nodes (line coverage)
      const nodeRegex = /<node\s+line="(\d+)"\s+count="(\d+)"/g;
      let nodeMatch;
      while ((nodeMatch = nodeRegex.exec(funcBody)) !== null) {
        const line = parseInt(nodeMatch[1], 10);
        const count = parseInt(nodeMatch[2], 10);
        const existingHits = fileData.lines.get(line) || 0;
        fileData.lines.set(line, Math.max(existingHits, count));
      }

      // Parse branches
      const branchMatch = /<branches\s+total="(\d+)"\s+executed="(\d+)"/.exec(funcBody);
      if (branchMatch) {
        fileData.branches.total += parseInt(branchMatch[1], 10);
        fileData.branches.executed += parseInt(branchMatch[2], 10);
      }
    }
  }
}

function generateReport(coverageData: CoverageData, verbose: boolean): number {
  let totalLines = 0;
  let coveredLines = 0;
  let totalFunctions = 0;
  let coveredFunctions = 0;
  let totalBranches = 0;
  let coveredBranches = 0;

  const fileStats: FileStat[] = [];

  const sortedFiles = Array.from(coverageData.files.entries()).sort((a, b) => a[0].localeCompare(b[0]));

  for (const [filepath, data] of sortedFiles) {
    const fileTotalLines = data.lines.size;
    const fileCoveredLines = Array.from(data.lines.values()).filter(h => h > 0).length;

    const fileTotalFuncs = data.functions.size;
    const fileCoveredFuncs = Array.from(data.functions.values()).filter(c => c > 0).length;

    totalLines += fileTotalLines;
    coveredLines += fileCoveredLines;
    totalFunctions += fileTotalFuncs;
    coveredFunctions += fileCoveredFuncs;
    totalBranches += data.branches.total;
    coveredBranches += data.branches.executed;

    const lineRate = fileTotalLines > 0 ? (fileCoveredLines / fileTotalLines) * 100 : 0;
    const funcRate = fileTotalFuncs > 0 ? (fileCoveredFuncs / fileTotalFuncs) * 100 : 0;

    fileStats.push({
      path: filepath,
      lines: fileTotalLines,
      covered: fileCoveredLines,
      lineRate,
      functions: fileTotalFuncs,
      funcCovered: fileCoveredFuncs,
      funcRate
    });
  }

  // Print report
  console.log('='.repeat(80));
  console.log('WinCC OA Code Coverage Report');
  console.log('='.repeat(80));
  console.log();

  if (verbose) {
    console.log(`${'File'.padEnd(60)} ${'Lines'.padStart(8)} ${'Cover'.padStart(8)} ${'Rate'.padStart(7)}`);
    console.log('-'.repeat(80));
    for (const stat of fileStats) {
      let shortPath = stat.path;
      if (shortPath.length > 58) {
        shortPath = '...' + shortPath.slice(-55);
      }
      console.log(`${shortPath.padEnd(60)} ${stat.lines.toString().padStart(8)} ${stat.covered.toString().padStart(8)} ${stat.lineRate.toFixed(1).padStart(6)}%`);
    }
    console.log('-'.repeat(80));
    console.log();
  }

  // Summary
  console.log('SUMMARY');
  console.log('-'.repeat(40));

  const overallLineRate = totalLines > 0 ? (coveredLines / totalLines) * 100 : 0;
  const overallFuncRate = totalFunctions > 0 ? (coveredFunctions / totalFunctions) * 100 : 0;
  const overallBranchRate = totalBranches > 0 ? (coveredBranches / totalBranches) * 100 : 0;

  console.log(`Files:     ${coverageData.files.size.toString().padStart(6)}`);
  console.log(`Lines:     ${coveredLines.toString().padStart(6)} / ${totalLines.toString().padEnd(6)} (${overallLineRate.toFixed(1)}%)`);
  console.log(`Functions: ${coveredFunctions.toString().padStart(6)} / ${totalFunctions.toString().padEnd(6)} (${overallFuncRate.toFixed(1)}%)`);
  console.log(`Branches:  ${coveredBranches.toString().padStart(6)} / ${totalBranches.toString().padEnd(6)} (${overallBranchRate.toFixed(1)}%)`);
  console.log();

  // Visual bar
  const barWidth = 50;
  const filled = Math.round(barWidth * overallLineRate / 100);
  const bar = '█'.repeat(filled) + '░'.repeat(barWidth - filled);
  console.log(`Coverage: [${bar}] ${overallLineRate.toFixed(1)}%`);
  console.log('='.repeat(80));

  return overallLineRate;
}

function generateCobertura(coverageData: CoverageData, outputPath: string): void {
  let totalLines = 0;
  let coveredLines = 0;
  let totalBranches = 0;
  let coveredBranches = 0;

  for (const data of coverageData.files.values()) {
    totalLines += data.lines.size;
    coveredLines += Array.from(data.lines.values()).filter(h => h > 0).length;
    totalBranches += data.branches.total;
    coveredBranches += data.branches.executed;
  }

  const lineRate = totalLines > 0 ? coveredLines / totalLines : 0;
  const branchRate = totalBranches > 0 ? coveredBranches / totalBranches : 0;

  let xml = `<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE coverage SYSTEM "http://cobertura.sourceforge.net/xml/coverage-04.dtd">
<coverage version="1.0" timestamp="${Date.now()}" lines-valid="${totalLines}" lines-covered="${coveredLines}" line-rate="${lineRate.toFixed(4)}" branches-valid="${totalBranches}" branches-covered="${coveredBranches}" branch-rate="${branchRate.toFixed(4)}" complexity="0">
  <packages>
    <package name="WinCC_OA" line-rate="${lineRate.toFixed(4)}" branch-rate="${branchRate.toFixed(4)}" complexity="0">
      <classes>
`;

  const sortedFiles = Array.from(coverageData.files.entries()).sort((a, b) => a[0].localeCompare(b[0]));

  for (const [filepath, data] of sortedFiles) {
    const fileLines = data.lines.size;
    const fileCovered = Array.from(data.lines.values()).filter(h => h > 0).length;
    const fileLineRate = fileLines > 0 ? fileCovered / fileLines : 0;
    const fileName = path.basename(filepath);

    xml += `        <class name="${escapeXml(fileName)}" filename="${escapeXml(filepath)}" line-rate="${fileLineRate.toFixed(4)}" branch-rate="0" complexity="0">
          <methods>
`;

    for (const [funcName, count] of data.functions) {
      xml += `            <method name="${escapeXml(funcName)}" signature="" line-rate="${count > 0 ? '1.0' : '0.0'}" branch-rate="0"/>
`;
    }

    xml += `          </methods>
          <lines>
`;

    const sortedLines = Array.from(data.lines.entries()).sort((a, b) => a[0] - b[0]);
    for (const [lineNum, hits] of sortedLines) {
      xml += `            <line number="${lineNum}" hits="${hits}"/>
`;
    }

    xml += `          </lines>
        </class>
`;
  }

  xml += `      </classes>
    </package>
  </packages>
</coverage>
`;

  fs.writeFileSync(outputPath, xml, 'utf8');
}

function escapeXml(str: string): string {
  return str
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&apos;');
}

function findCoverageFiles(dirPath: string): string[] {
  const files: string[] = [];
  try {
    const entries = fs.readdirSync(dirPath);
    for (const entry of entries) {
      if (entry.startsWith('CoverageReport_') && entry.endsWith('.xml')) {
        files.push(path.join(dirPath, entry));
      }
    }
  } catch (e) {
    // Directory not accessible
  }
  return files;
}

function main(): number {
  const args = process.argv.slice(2);
  let inputPath = '.';
  let filterPath: string | undefined;
  let verbose = false;
  let outputPath: string | undefined;

  // Parse arguments
  for (let i = 0; i < args.length; i++) {
    if (args[i] === '-f' || args[i] === '--filter') {
      filterPath = args[++i];
    } else if (args[i] === '-v' || args[i] === '--verbose') {
      verbose = true;
    } else if (args[i] === '-o' || args[i] === '--output') {
      outputPath = args[++i];
    } else if (!args[i].startsWith('-')) {
      inputPath = args[i];
    }
  }

  // Find coverage files
  let xmlFiles: string[];
  if (fs.existsSync(inputPath) && fs.statSync(inputPath).isFile()) {
    xmlFiles = [inputPath];
  } else {
    xmlFiles = findCoverageFiles(inputPath);
  }

  if (xmlFiles.length === 0) {
    console.log(`No coverage files found in ${inputPath}`);
    return 1;
  }

  console.log(`Processing ${xmlFiles.length} coverage file(s)...`);
  console.log();

  // Parse all coverage files
  const coverageData: CoverageData = { files: new Map() };
  for (const xmlFile of xmlFiles) {
    try {
      const content = fs.readFileSync(xmlFile, 'utf8');
      parseCoverageXml(content, coverageData, filterPath);
    } catch (e) {
      console.log(`Warning: Could not read ${xmlFile}`);
    }
  }

  // Generate report
  const overallRate = generateReport(coverageData, verbose);

  // Optionally generate Cobertura XML
  if (outputPath) {
    generateCobertura(coverageData, outputPath);
    console.log(`\nCobertura XML written to: ${outputPath}`);
  }

  return 0;
}

process.exit(main());
