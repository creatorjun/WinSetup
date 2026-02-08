# scripts/check_dependencies.py

import os
import re
import sys
from pathlib import Path
from typing import List, Tuple, Set

class DependencyChecker:
    def __init__(self, root_path: str):
        self.root_path = Path(root_path)
        self.src_path = self.root_path / "src"
        self.violations: List[Tuple[str, str]] = []
        
        self.forbidden_domain_includes = [
            r'<[Ww]indows\.h>',
            r'<winnt\.h>',
            r'<winbase\.h>',
            r'<windef\.h>',
            r'<winnls\.h>',
            r'<wincon\.h>',
            r'<winuser\.h>',
            r'<wingdi\.h>',
            r'<fileapi\.h>',
            r'<handleapi\.h>',
            r'<processthreadsapi\.h>',
            r'<synchapi\.h>',
            r'<memoryapi\.h>',
            r'<winioctl\.h>',
            r'<commctrl\.h>',
            r'".*[Ww]in32.*"',
            r'".*[Aa]dapters.*"',
            r'".*[Aa]pplication.*"'
        ]
        
        self.forbidden_abstractions_includes = [
            r'<[Ww]indows\.h>',
            r'".*[Aa]dapters.*"',
            r'".*[Aa]pplication.*"',
            r'".*\.cpp"'
        ]
        
        self.forbidden_application_includes = [
            r'".*[Aa]dapters.*"',
            r'<[Ww]indows\.h>'
        ]

    def check_file(self, file_path: Path, layer: str) -> None:
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                content = f.read()
                lines = content.split('\n')
                
                for line_num, line in enumerate(lines, 1):
                    stripped = line.strip()
                    if not stripped.startswith('#include'):
                        continue
                    
                    if layer == 'domain':
                        self._check_domain_violations(file_path, line_num, stripped)
                    elif layer == 'abstractions':
                        self._check_abstractions_violations(file_path, line_num, stripped)
                    elif layer == 'application':
                        self._check_application_violations(file_path, line_num, stripped)
                        
        except Exception as e:
            print(f"Warning: Could not read {file_path}: {e}")

    def _check_domain_violations(self, file_path: Path, line_num: int, line: str) -> None:
        for pattern in self.forbidden_domain_includes:
            if re.search(pattern, line, re.IGNORECASE):
                self.violations.append((
                    str(file_path.relative_to(self.root_path)),
                    f"Line {line_num}: DOMAIN LAYER VIOLATION - Forbidden include detected: {line}"
                ))
                
        if 'application' in line.lower() and '#include' in line:
            self.violations.append((
                str(file_path.relative_to(self.root_path)),
                f"Line {line_num}: DOMAIN -> APPLICATION dependency violation: {line}"
            ))
            
        if 'adapters' in line.lower() and '#include' in line:
            self.violations.append((
                str(file_path.relative_to(self.root_path)),
                f"Line {line_num}: DOMAIN -> ADAPTERS dependency violation: {line}"
            ))

    def _check_abstractions_violations(self, file_path: Path, line_num: int, line: str) -> None:
        for pattern in self.forbidden_abstractions_includes:
            if re.search(pattern, line, re.IGNORECASE):
                self.violations.append((
                    str(file_path.relative_to(self.root_path)),
                    f"Line {line_num}: ABSTRACTIONS LAYER VIOLATION - Forbidden include: {line}"
                ))
                
        if file_path.suffix == '.cpp':
            self.violations.append((
                str(file_path.relative_to(self.root_path)),
                f"ABSTRACTIONS LAYER VIOLATION - Implementation file (.cpp) not allowed in abstractions"
            ))

    def _check_application_violations(self, file_path: Path, line_num: int, line: str) -> None:
        for pattern in self.forbidden_application_includes:
            if re.search(pattern, line, re.IGNORECASE):
                if 'adapters' in line.lower():
                    self.violations.append((
                        str(file_path.relative_to(self.root_path)),
                        f"Line {line_num}: APPLICATION -> ADAPTERS dependency violation: {line}"
                    ))
                elif 'windows.h' in line.lower():
                    self.violations.append((
                        str(file_path.relative_to(self.root_path)),
                        f"Line {line_num}: APPLICATION LAYER - Windows API usage not allowed: {line}"
                    ))

    def scan_directory(self, directory: Path, layer: str) -> None:
        if not directory.exists():
            return
            
        for file_path in directory.rglob('*.h'):
            self.check_file(file_path, layer)
        for file_path in directory.rglob('*.cpp'):
            self.check_file(file_path, layer)

    def run(self) -> int:
        print("=" * 80)
        print("Clean Architecture Dependency Checker")
        print("=" * 80)
        print(f"Scanning: {self.src_path}\n")
        
        domain_path = self.src_path / "domain"
        abstractions_path = self.src_path / "abstractions"
        application_path = self.src_path / "application"
        
        print("Checking Domain layer...")
        self.scan_directory(domain_path, 'domain')
        
        print("Checking Abstractions layer...")
        self.scan_directory(abstractions_path, 'abstractions')
        
        print("Checking Application layer...")
        self.scan_directory(application_path, 'application')
        
        if self.violations:
            print("\n" + "=" * 80)
            print(f"DEPENDENCY VIOLATIONS FOUND: {len(self.violations)}")
            print("=" * 80)
            
            for file_path, message in self.violations:
                print(f"\n[ERROR] {file_path}")
                print(f"  {message}")
            
            print("\n" + "=" * 80)
            print("Build aborted due to architecture violations!")
            print("=" * 80)
            return 1
        else:
            print("\n" + "=" * 80)
            print("✓ All dependency checks passed!")
            print("=" * 80)
            return 0

def main():
    if len(sys.argv) < 2:
        print("Usage: python check_dependencies.py <project_root_path>")
        sys.exit(1)
    
    project_root = sys.argv[1]
    
    if not os.path.exists(project_root):
        print(f"Error: Path does not exist: {project_root}")
        sys.exit(1)
    
    checker = DependencyChecker(project_root)
    exit_code = checker.run()
    sys.exit(exit_code)

if __name__ == "__main__":
    main()
