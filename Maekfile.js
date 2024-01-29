//Run this (javascript) file with node:
//$ node Maekfile.js [-jN] [-v] [-q] [--] [target1] [target2] [...]
//
// Maekfile.js will [re-]build all the specified targets.
// Results are re-used by content hash if they match "maek-cache.json"; delete that file to force a full rebuild.
//
//Command line options:
//  -jN      limit on parallel jobs; defaults to number of cpu cores + 1
//  -v       verbose output; prints more info
//  -q       quit on first error (otherwise builds as much as possible)
//  --       optional separator between command line switches and targets (useful if you have a target named '-j1')
//  targetN  target name. Posix-style path to a file to build, or an abstract target (word starting with ':')
//

//maek is configured using properties and methods of the `maek` object:
const maek = init_maek();
// (it's a quirk of javascript that function definitions anywhere in scope get 'hoisted'
//   -- you can see the definition of init_maek by scrolling down.)

//Read onward to discover how to configure Maek for your build!
//======================================================================

//set default targets to build (can be overridden by command line options):
maek.TARGETS = ["Scotty3D" + (maek.OS === "windows" ? ".exe" : "")];

//we use a prebuilt library package:
const NEST_LIBS = `nest-libs/${maek.OS}`;

//make sure the user has actually downloaded the library package(!):
function check_for_nest() {
	const fs = require('fs');
	try {
		const stats = fs.statSync(NEST_LIBS);
		if (!stats.isDirectory()) throw new Error(`${NEST_LIBS} is not a directory.`);
	} catch (e) {
		console.error(`${e}\n\nLooks like the folder '${NEST_LIBS}' does not exist -- this probably means you haven't downloaded and extracted the nest-libs package to the correct spot.\nSee the README.md file for more details.\n`);
		process.exit(1);
	}
}
check_for_nest();

//set compile flags (these can also be overridden per-task using the "options" parameter):
if (maek.OS === "windows") {
	maek.options.CPPFlags.push(
		"/O2", //optimize
		//include paths for pre-built libraries:
		`/I${NEST_LIBS}/SDL2/include`,
		//include directories:
		"/Isrc", "/Ideps",
		//other flags:
		"/DWIN32_LEAN_AND_MEAN", //make windows.h pollute the global namespace less
		"/wd4100", //unreferenced formal parameter
		"/wd4146", //-1U is unsigned
		"/wd4201", //nameless struct/union
		"/wd4840"  //string as variadic function argument
	);
} else if (maek.OS === "linux") {
	maek.options.CPPFlags.push(
		"-O2", //optimize
		`-I${NEST_LIBS}/SDL2/include/SDL2`, `-D_THREAD_SAFE`, //SDL include flags
		"-Isrc", "-Ideps"  //include directories
	);
} else if (maek.OS === "macos") {
	maek.options.CPPFlags.push(
		"-O2", //optimize
		`-I${NEST_LIBS}/SDL2/include/SDL2`, `-D_THREAD_SAFE`, //SDL include flags
		"-Isrc", "-Ideps"  //include directories
	);
}

//call rules on the maek object to specify tasks.
// rules generally look like:
//  output = maek.RULE_NAME(input [, output] [, {options}])

//use COPY to copy a file
// 'COPY(from, to)'
// from: file to copy from
// to: file to copy to
let copies = [
	maek.COPY(`${NEST_LIBS}/SDL2/dist/README-SDL.txt`, `README-SDL.txt`),
];
if (maek.OS === 'windows') {
	copies.push( maek.COPY(`${NEST_LIBS}/SDL2/dist/SDL2.dll`, `SDL2.dll`) );
}

//Compile a c++ file with CPP:
//[objFile =] CPP(cppFile [, objFileBase] [, options])
// cppFile: name of c++ file to compile
// objFileBase (optional): base name object file to produce (if not supplied, set to options.objDir + '/' + cppFile without the extension)
// returns objFile: objFileBase + a platform-dependant suffix ('.o' or '.obj')

const gui_objects = [
	maek.CPP("src/gui/manager.cpp"),
	maek.CPP("src/gui/widgets.cpp"),
	maek.CPP("src/gui/model.cpp"),
	maek.CPP("src/gui/render.cpp"),
	maek.CPP("src/gui/rig.cpp"),
	maek.CPP("src/gui/simulate.cpp"),
	maek.CPP("src/gui/animate.cpp"),
];
const geom_objects = [
	maek.CPP("src/geometry/halfedge-utility.cpp"),
	maek.CPP("src/geometry/halfedge-local.cpp"),
	maek.CPP("src/geometry/halfedge-global.cpp"),
	maek.CPP("src/geometry/indexed.cpp"),
	maek.CPP("src/geometry/util.cpp"),
	maek.CPP("src/geometry/spline.cpp"),
];
const rasterizer_objects = [
	maek.CPP("src/rasterizer/pipeline.cpp"),
	maek.CPP("src/rasterizer/rasterizer.cpp"),
	maek.CPP("src/rasterizer/framebuffer.cpp"),
	maek.CPP("src/rasterizer/sample_pattern.cpp"),
];
const pathtracer_objects = [
	maek.CPP("src/pathtracer/pathtracer.cpp"),
	maek.CPP("src/pathtracer/tri_mesh.cpp"),
	maek.CPP("src/pathtracer/bvh.cpp"),
	maek.CPP("src/pathtracer/samplers.cpp"),
	maek.CPP("src/pathtracer/aperture_shape.cpp"),
];
const util_objects = [
	maek.CPP("src/util/hdr_image.cpp"),
	maek.CPP("src/util/viewer.cpp"),
	maek.CPP("src/util/thread_pool.cpp"),
	maek.CPP("src/util/rand.cpp"),
	maek.CPP("src/util/timer.cpp"),
	maek.CPP("src/util/to_json.cpp"),
];
const platform_objects = [
	maek.CPP("src/platform/gl.cpp"),
	maek.CPP("src/platform/renderer.cpp"),
	maek.CPP("src/platform/platform.cpp"),
];
const scene_objects = [
	maek.CPP("src/scene/undo.cpp"),
	maek.CPP("src/scene/scene.cpp"),
	maek.CPP("src/scene/scene-step.cpp"),
	maek.CPP("src/scene/load-save.cpp"),
	maek.CPP("src/scene/load-save-json.cpp"),
	maek.CPP("src/scene/io.cpp"),
	maek.CPP("src/scene/animator.cpp"),
	maek.CPP("src/scene/delta_light.cpp"),
	maek.CPP("src/scene/env_light.cpp"),
	maek.CPP("src/scene/material.cpp"),
	maek.CPP("src/scene/shape.cpp"),
	maek.CPP("src/scene/skeleton.cpp"),
	maek.CPP("src/scene/transform.cpp"),
	maek.CPP("src/scene/particles.cpp"),
	maek.CPP("src/scene/texture.cpp"),
	maek.CPP("src/scene/camera.cpp"),
];

//Some of the dependencies have special build needs:

const sf_libs_options = {};
const nfd_common_options = {};
if (maek.OS === 'windows') {
	sf_libs_options.CPPFlags = [...maek.options.CPPFlags,
		"/wd4018", //signed-unsigned mismatch (compare)
		"/wd4389", //signed-unsigned mismatch (equal)
		"/wd4996", //signed-unsigned mismatch (return)
	];
	nfd_common_options.CPPFlags = [...maek.options.CPPFlags,
		"/D_CRT_SECURE_NO_WARNINGS", //using strncpy
		"/wd4245", //signed-unsigned mismatch (return)
	];
} else {
	sf_libs_options.CPPFlags = [...maek.options.CPPFlags,
		"-Wno-sign-compare",
		"-Wno-unused-but-set-variable",
	];
	if (maek.OS === "macos") {
		nfd_common_options.CPP = ['clang', '-Wall', '-Werror', '-g'];
	}
}

const deps_objects = [
	maek.CPP("deps/sf_libs/sf_libs.cpp", undefined, sf_libs_options),
	maek.CPP("deps/imgui/imgui_impl_opengl3.cpp"),
	maek.CPP("deps/imgui/imgui_impl_sdl.cpp"),
	maek.CPP("deps/imgui/imgui_tables.cpp"),
	maek.CPP("deps/imgui/imgui_widgets.cpp"),
	maek.CPP("deps/imgui/imgui_draw.cpp"),
	maek.CPP("deps/imgui/imgui.cpp"),
	maek.CPP("deps/glad/glad.cpp"),
	maek.CPP("deps/nfd/nfd_common.c", undefined, nfd_common_options),
	maek.CPP("deps/sejp/sejp.cpp"),
];
if (maek.OS === 'linux') {
	deps_objects.push(
		maek.CPP("deps/nfd/nfd_gtk.c", undefined, {
			CPP: ['gcc', '-Wall', '-Werror', '-g'],
			CPPFlags: [...maek.options.CPPFlags,
				"-I/usr/include/gtk-3.0",
				"-I/usr/include/glib-2.0",
				"-I/usr/lib/x86_64-linux-gnu/glib-2.0/include",
				"-I/usr/include/pango-1.0",
				"-I/usr/include/harfbuzz",
				"-I/usr/include/cairo",
				"-I/usr/include/gdk-pixbuf-2.0",
				"-I/usr/include/atk-1.0",
				'-Wno-format-truncation'
			]
		})
	);
} else if (maek.OS === 'windows') {
	deps_objects.push(
		maek.CPP("deps/nfd/nfd_win.cpp", undefined, {
			CPPFlags: [...maek.options.CPPFlags,
				"/wd4189", //local variable not referenced
				"/wd4701", //potentially uninitialized local variable
				"/wd4703", //potentially uninitialized local pointer variable
				"/wd4389", //signed/unsigned mismatch (only on x86 not x64)
				"/D_CRT_SECURE_NO_WARNINGS", //using strncpy
			]
		})
	);
} else if (maek.OS === 'macos') {
	deps_objects.push(
		maek.CPP("deps/nfd/nfd_cocoa.m", undefined, {
			CPP: ['clang', '-ObjC', '-Wall', '-Werror', '-g'],
			CPPFlags: [...maek.options.CPPFlags, "-Wno-deprecated-declarations"]
		})
	);
}

//compile every .cpp under "tests":
function allTests(dir = "tests") {
	const ret = [];
	const fs = require('fs');
	for (const ent of fs.readdirSync(dir, { withFileTypes: true })) {
		if (ent.isDirectory()) {
			ret.push(...allTests(dir + "/" + ent.name));
		} else if (ent.isFile() && ent.name.endsWith(".cpp")) {
			ret.push(maek.CPP(dir + "/" + ent.name));
		}
	}
	return ret;
}
const test_objects = allTests();

const Scotty3D_objects = [
	...gui_objects,
	...geom_objects,
	...rasterizer_objects,
	...pathtracer_objects,
	...util_objects,
	...platform_objects,
	...scene_objects,
	...deps_objects,
	...test_objects,
	maek.CPP("src/app.cpp"),
	maek.CPP("src/main.cpp"),
	maek.CPP("src/test.cpp")
];

//link object files with LINK:
//[exeFile =] LINK(objFiles, exeFileBase, [, options])
// objFiles: array of objects to link
// exeFileBase: name of executable file to produce
// returns exeFile: exeFileBase + a platform-dependant suffix (e.g., '.exe' on windows)

const Scotty3D_options = {};
if (maek.OS === 'linux') {
	Scotty3D_options.LINKLibs = [...maek.options.LINKLibs,
		`-L${NEST_LIBS}/SDL2/lib`, `-lSDL2`, `-lm`, `-ldl`, `-lasound`, `-lpthread`, `-lX11`, `-lXext`, `-lpthread`, `-lrt`, `-lGL`,
		"-L/usr/lib/x86_64-linux-gnu/", "-lgtk-3", "-lgobject-2.0", "-lglib-2.0", "-ldl",
		"-pthread",
	];
} else if (maek.OS === 'macos') {
	Scotty3D_options.LINKLibs = [...maek.options.LINKLibs,
		`-L${NEST_LIBS}/SDL2/lib`, `-lSDL2`, `-lm`,`-liconv`, `-framework`, `CoreAudio`, `-framework`, `AudioToolbox`, `-weak_framework`, `CoreHaptics`, `-weak_framework`, `GameController`, `-framework`, `ForceFeedback`, `-lobjc`, `-framework`, `CoreVideo`, `-framework`, `Cocoa`, `-framework`, `Carbon`, `-framework`, `IOKit`, `-framework`, `OpenGL`,
		"-framework", "Foundation",
		"-framework", "AppKit",
	];
} else if (maek.OS === 'windows') {
	Scotty3D_options.LINKLibs = [...maek.options.LINKLibs,
		`/LIBPATH:${NEST_LIBS}/SDL2/lib`, `SDL2main.lib`, `SDL2.lib`, `OpenGL32.lib`, `Shell32.lib`, //needed for SDL
		"Ole32.lib", "Shcore.lib", //needed for NFD
		"src/platform/icon.res",
	];
}

//make Scotty3D executable also depend on copying the correct dynamic libraries and readme file from the nest-libs package:
Scotty3D_options.depends = [...copies];

const Scotty3D_exe = maek.LINK(Scotty3D_objects, 'Scotty3D', Scotty3D_options);

//======================================================================
//Now, onward to the code that makes all this work:

function init_maek() {
	//----------------------------------
	//some setup

	//standard libraries:
	const path = require('path').posix; //NOTE: expect posix-style paths even on windows
	const fsPromises = require('fs/promises');
	const fs = require('fs');
	const os = require('os');
	const performance = require('perf_hooks').performance;
	const child_process = require('child_process');

	//make it so that all paths/commands are relative to this file:
	// (regardless of where you run it from)
	console.log(`Building in ${__dirname}.`);
	process.chdir(__dirname);

	//make it slightly more idiomatic to export:
	const maek = module.exports;

	//-----------------------------------------
	//Constants:

	//cache file location:
	maek.CACHE_FILE = 'maek-cache.json';

	//current OS: (with slightly nicer naming than os.platform()
	maek.OS = (() => {
		const platform = os.platform();
		if (platform === 'win32') return 'windows';
		else if (platform === 'darwin') return 'macos';
		else if (platform === 'linux') return 'linux';
		else {
			console.error(`ERROR: Unrecognized platform ${os.platform()}.`);
			process.exit(1);
		}
	})();

	//-----------------------------------------
	//Command line defaults:

	//maximum number of jobs to run: (change with -jN)
	maek.JOBS = os.cpus().length + 1;

	//targets to build by default: (change by passing target names)
	maek.TARGETS = [];

	//print extra info: (set by passing -v)
	maek.VERBOSE = false;

	//quit on first failure: (set by passing -q)
	maek.QUIT_EAGERLY = false;

	//-----------------------------------------
	//options: set to change maek rule behavior

	const DEFAULT_OPTIONS = {
		objPrefix: 'objs/', //prefix for object file paths (if not explicitly specified)
		objSuffix: (maek.OS === 'windows' ? '.obj' : '.o'), //suffix for object files
		exeSuffix: (maek.OS === 'windows' ? '.exe' : ''), //suffix for executable files
		depends: [], //extra dependencies; generally only set locally
		CPP: [], //the c++ compiler and any flags to start with (set below, per-OS)
		CPPFlags: [], //extra flags for c++ compiler
		LINK: [], //the linker and any flags to start with (set below, per-OS)
		LINKLibs: [], //extra -L and -l flags for linker
	}

	if (maek.OS === 'windows') {
		DEFAULT_OPTIONS.CPP = ['cl.exe', '/nologo', '/EHsc', '/Z7', '/std:c++17', '/W4', '/WX', '/MD'];
		//TODO: could embed manifest to set UTF8 codepage
		DEFAULT_OPTIONS.LINK = ['link.exe', '/nologo', '/SUBSYSTEM:CONSOLE', '/DEBUG:FASTLINK', '/INCREMENTAL:NO'];
	} else if (maek.OS === 'linux') {
		DEFAULT_OPTIONS.CPP = ['g++', '-std=c++17', '-Wall', '-Werror', '-g'];
		DEFAULT_OPTIONS.LINK = ['g++', '-std=c++17', '-Wall', '-Werror', '-g'];
	} else if (maek.OS === 'macos') {
		DEFAULT_OPTIONS.CPP = ['clang++', '-std=c++17', '-Wall', '-Werror', '-g'];
		DEFAULT_OPTIONS.LINK = ['clang++', '-std=c++17', '-Wall', '-Werror', '-g'];
	}

	//any settings here override 'DEFAULT_OPTIONS':
	maek.options = Object.assign({}, DEFAULT_OPTIONS); //shallow copy of DEFAULT_OPTIONS in case you want to console.log(maek.options) to check settings.

	//this combines DEFAULT_OPTIONS, maek.options, and localOptions:
	function combineOptions(localOptions) {
		//shallow copy of default options:
		const combined = Object.assign({}, DEFAULT_OPTIONS);
		//override with maek.options + complain on missing keys:
		for (const key of Object.keys(maek.options)) {
			if (!(key in combined)) throw new Error(`ERROR: '${key}' (in maek.options) not recognized.`);
			combined[key] = maek.options[key];
		}
		//override with localOptions + complain on missing keys:
		for (const key of Object.keys(localOptions)) {
			if (!(key in combined)) throw new Error(`ERROR: '${key}' (in local options) not recognized.`);
			combined[key] = localOptions[key];
		}
		return combined;
	}

	//tasks is a map from targets -> tasks:
	maek.tasks = {};

	//-----------------------------------------
	//RULES.
	// helper functions that specify tasks:

	//COPY adds a task that copies a file:
	maek.COPY = (srcFile, dstFile) => {
		if (typeof srcFile !== "string") throw new Error("COPY: from should be a single file.");
		if (typeof dstFile !== "string") throw new Error("COPY: to should be a single file.");
		const task = async () => {
			try {
				await fsPromises.mkdir(path.dirname(dstFile), { recursive: true });
				await fsPromises.copyFile(srcFile, dstFile);
			} catch (e) {
				throw new BuildError(`Failed to copy '${srcFile}' to '${dstFile}': ${e}`);
			}
		};
		task.depends = [srcFile];
		task.label = `COPY ${dstFile}`;
		maek.tasks[dstFile] = task;

		return dstFile;
	};


	//maek.CPP makes an object from a c++ source file:
	// cppFile is the source file name
	// objFileBase (optional) is the output file (including any subdirectories, but not the extension)
	maek.CPP = (cppFile, objFileBase, localOptions = {}) => {
		//combine options:
		const options = combineOptions(localOptions);

		//if objFileBase isn't given, compute by trimming extension from cppFile and appending to objPrefix:
		if (typeof objFileBase === 'undefined') {
			objFileBase = path.relative('', options.objPrefix + cppFile.replace(/\.[^.]*$/, ''));
		}

		//object file gets os-dependent suffix:
		const objFile = objFileBase + options.objSuffix;

		//computed dependencies go in a '.d' file stored next to the object file:
		const depsFile = objFileBase + '.d';

		let cc, command;
		cc = [...options.CPP, ...options.CPPFlags];
		if (maek.OS === 'linux') {
			//TODO: check on linux
			command = [...cc, '-MD', '-MT', 'x ', '-MF', depsFile, '-c', '-o', objFile, cppFile];
		} else if (maek.OS === 'macos') {
			//TODO: check on macos
			command = [...cc, '-MD', '-MT', 'x ', '-MF', depsFile, '-c', '-o', objFile, cppFile];
		} else { //windows
			command = [...cc, '/c', `/Fo${objFile}`, '/sourceDependencies', depsFile, '/Tp', cppFile];
		}

		//will be used by loadDeps to trim explicit dependencies:
		async function loadDeps() {
			const text = await fsPromises.readFile(depsFile, { encoding: 'utf8' });

			if (maek.OS === 'windows') {
				//parse JSON-encoded dependency info from /sourceDependencies:
				const winpath = require('path').win32;
				const parsed = JSON.parse(text);
				let paths = [...parsed.Data.Includes, parsed.Data.Source];
				paths = paths.map(path => winpath.relative('', path).split('\\').join('/'));
				paths = paths.sort();
				return paths;
			} else {
				//parse the makefile-style "targets : prerequisites" line from the file into a list of tokens:
				let tokens = text
					.replace(/\\?\n/g, ' ') //escaped newline (or regular newline) => whitespace
					.trim() //remove leading and trailing whitespace
					.replace(/([^\\])\s+/g, '$1\n') //run of non-escaped whitespace => single newline
					.split('\n'); //split on single newlines

				//because of the `-MT 'x '` option, expect 'x :' at the start of the rule:
				console.assert(tokens[0] === 'x');
				console.assert(tokens[1] === ':');
				tokens = tokens.slice(2); //remove the 'x :'
				//tokens = tokens.map(path => path.relative('', path)); //hmmm does this do anything worthwhile?
				tokens = tokens.sort(); //sort for consistency

				//NOTE: might want to do some path normalization here!
				return tokens;
			}
		}

		//The actual build task:
		const task = async () => {
			//make object file:
			await fsPromises.mkdir(path.dirname(objFile), { recursive: true });
			await fsPromises.mkdir(path.dirname(depsFile), { recursive: true });
			await run(command, `${task.label}: compile + prerequisites`,
				async () => {
					return {
						read:[...await loadDeps()],
						written:[objFile, depsFile]
					};
				}
			);
		};

		task.depends = [cppFile, ...options.depends];

		task.label = `CPP ${objFile}`;

		if (objFile in maek.tasks) {
			throw new Error(`Task ${task.label} purports to create ${objFile}, but ${maek.tasks[objFile].label} already creates that file.`);
		}
		maek.tasks[objFile] = task;

		return objFile;
	};

	//maek.LINK links an executable file from a collection of object files:
	// objFiles is an array of object file names
	// exeFileBase is the base name of the executable file ('.exe' will be added on windows)
	maek.LINK = (objFiles, exeFileBase, localOptions = {}) => {
		const options = combineOptions(localOptions);

		const exeFile = exeFileBase + options.exeSuffix;

		let link, linkCommand;
		link = [...options.LINK];
		if (maek.OS === 'linux') {
			linkCommand = [...link, '-o', exeFile, ...objFiles, ...options.LINKLibs];
		} else if (maek.OS === 'macos') {
			linkCommand = [...link, '-o', exeFile, ...objFiles, ...options.LINKLibs];
		} else {
			linkCommand = [...link, `/out:${exeFile}`, ...objFiles, ...options.LINKLibs];
		}

		const task = async () => {
			await fsPromises.mkdir(path.dirname(exeFile), { recursive: true });
			await run(linkCommand, `${task.label}: link`,
				async () => {
					return {
						read:[...objFiles],
						written:[exeFile]
					};
				}
			);
		};

		task.depends = [...objFiles, ...options.depends];
		task.label = `LINK ${exeFile}`;

		if (exeFile in maek.tasks) {
			throw new Error(`Task ${task.label} purports to create ${exeFile}, but ${maek.tasks[exeFile].label} already creates that file.`);
		}
		maek.tasks[exeFile] = task;

		return exeFile;
	};


	//says something went wrong in building -- should fail loudly:
	class BuildError extends Error {
		constructor(message) {
			super(message);
		}
	}

	//cache stores the hashes of files involved in run()'d commands:
	let cache = {};

	function loadCache() {
		try {
			const loaded = JSON.parse(fs.readFileSync(maek.CACHE_FILE, { encoding: 'utf8' }));
			let assigned = 0;
			let removed = 0;
			for (const command of Object.keys(loaded)) {
				//cache will have a 'files' and a 'hashes' line
				if ('files' in loaded[command] && 'hashes' in loaded[command]) {
					cache[command] = {
						files:loaded[command].files,
						hashes:loaded[command].hashes
					};
					assigned += 1;
				} else {
					removed += 1;
				}
			}
			if (maek.VERBOSE) console.log(` -- Loaded cache from '${maek.CACHE_FILE}'; had ${assigned} valid entries and ${removed} invalid ones.`);
		} catch (e) {
			if (maek.VERBOSE) console.log(` --  No cache loaded; starting fresh.`);
			if (e.code !== 'ENOENT') {
				console.warn(`Cache loading failed for unexpected reason:`, e);
			}
		}
	}

	function saveCache() {
		if (maek.VERBOSE) console.log(` -- Writing cache with ${Object.keys(cache).length} entries to '${maek.CACHE_FILE}'...`);
		fs.writeFileSync(maek.CACHE_FILE, JSON.stringify(cache), { encoding: 'utf8' });
	}

	let runTime = 0.0;

	//runs a shell command (presented as an array)
	// 'message' will be displayed above the command
	// 'cacheInfoFn', if provided, will be called after function is run to determine which files to hash when caching the result
	async function run(command, message, cacheInfoFn) {

		//cache key for the command -- encoded command name:
		const cacheKey = JSON.stringify(command);

		//executable for the command:
		const exe = await findExe(command);

		//if no cache info function, remove any existing cache entry:
		if (!cacheInfoFn) {
			delete cache[cacheKey];
		}

		//check for existing cache entry:
		let extra = ''; //extra message
		if (cacheKey in cache) {
			const cached = cache[cacheKey].hashes;
			const current = await hashFiles([exe, ...cache[cacheKey].files]);
			if (JSON.stringify(current) === JSON.stringify(cached)) {
				if (maek.VERBOSE) console.log(`\x1b[33m${message} [cached]\x1b[0m`);
				return;
			} else {
				if (maek.VERBOSE) extra = ` \x1b[33m[cache miss!]\x1b[0m`;
			}
		}


		if (typeof message !== 'undefined') {
			console.log(`\x1b[90m${message}\x1b[0m${extra}`);
		}

		//print a command in a way that can be copied to a shell to run:
		let prettyCommand = '';
		for (const token of command) {
			if (prettyCommand !== '') prettyCommand += ' ';
			if (/[ \t\n!"'$&()*,;<>?[\\\]^`{|}~]/.test(token)
				|| token[0] === '='
				|| token[0] === '#') {
				//special characters => need to quote:
				prettyCommand += "'" + token.replace(/'/g, "'\\''") + "'";
			} else {
				prettyCommand += token;
			}
		}
		console.log('   ' + prettyCommand);

		//package as a promise and await it finishing:
		const before = performance.now();
		await new Promise((resolve, reject) => {
			const proc = child_process.spawn(command[0], command.slice(1), {
				shell: false,
				stdio: ['ignore', 'inherit', 'inherit']
			});
			proc.on('exit', (code, signal) => {
				if (code !== 0) {
					process.stderr.write(`\n`);
					reject(new BuildError(`exit ${code} from:\n    \x1b[31m${prettyCommand}\x1b[0m\n`));
				} else {
					resolve();
				}
			});
			proc.on('error', (err) => {
				reject(new BuildError(`${err.message} from:\n    ${prettyCommand}`));
			});
		});
		runTime += performance.now() - before;

		//store result in cache:
		if (cacheInfoFn) {
			const {read, written} = await cacheInfoFn();

			//if hashed one of the written files before, can't rely on it:
			for (const file of written) {
				delete hashCache[file];
			}

			//update cache with file content hashes:
			const files = [...read, ...written];
			cache[cacheKey] = {
				files:files,
				hashes:await hashFiles([exe, ...files])
			};
		}

	}

	let hashCacheHits = 0;
	let hashCache = {};
	let hashLoadTime = 0.0;
	let hashComputeTime = 0.0;

	//hash a list of files and return a list of strings describing said hashes (or 'x' on missing file):
	async function hashFiles(files) {
		const crypto = require('crypto');

		//helper that will hash a single file: (non-existent files get special hash 'x')
		async function hashFile(file) {
			if (file in hashCache) {
				hashCacheHits += 1;
				return hashCache[file];
			}

			//would likely be more efficient to use a pipe with large files,
			//but this code is a bit more readable:
			const hash = await new Promise((resolve, reject) => {
				const beforeLoad = performance.now();
				fs.readFile(file, (err, data) => {
					hashLoadTime += performance.now() - beforeLoad;
					if (err) {
						//if failed to read file, report hash as 'x':
						if (err.code != "ENOENT") {
							console.warn(`Failed to hash ${file} because of unexpected error ${err}`); //DEBUG
						}
						resolve(`x`);
					} else {
						const beforeHash = performance.now();
						//otherwise, report base64-encoded md5sum of file data:
						const hash = crypto.createHash('md5');
						hash.update(data);
						resolve(`${hash.digest('base64')}`);
						hashComputeTime += performance.now() - beforeHash;
					}
				});
			});

			hashCache[file] = hash;
			return hash;
		}

		//get all hashes:
		const hashes = [];
		for (let file of files) {
			hashes.push(await hashFile(file));
		}
		return hashes;
	}

	//find an executable in the system path
	// (used by run to figure out what to hash)
	async function findExe(command) {
		const osPath = require('path');
		let PATH;
		if (maek.OS === 'windows') {
			PATH = process.env.PATH.split(';');
		} else {
			PATH = process.env.PATH.split(':');
		}
		for (const prefix of PATH) {
			const exe = osPath.resolve(prefix, command[0]);
			try {
				await fsPromises.access(exe, fs.constants.X_OK);
				return exe;
			} catch (e) {
				if (e.code === 'ENOENT') continue;
				else throw e;
			}
		}
		throw new BuildError(`Couldn't find file for command '${command[0]}'`);
		return "?";
	}

	//---------------------------------------
	//'update' actually runs tasks to make targets:

	maek.update = async (targets) => {
		const before = performance.now();
		console.log(` -- Maek v0.2 on ${maek.OS} with ${maek.JOBS} max jobs updating '${targets.join("', '")}'...`);

		loadCache();
		process.on('SIGINT', () => {
			console.log(`\x1b[91m!!! FAILED: interrupted\x1b[0m`);
			saveCache();
			process.exit(1);
		}); //allow saving cache on abort

		const tasks = maek.tasks;

		//clear temporary per-task data:
		for (const target in tasks) {
			delete tasks[target].neededBy; //which tasks need this task
			delete tasks[target].finished; //is this task finished?
			delete tasks[target].failed; //has this task failed?
		}


		//list of all tasks to run:
		const pending = [];

		//add to list of tasks to run and make neededBy array:
		function need(target, from) {
			if (!(target in tasks)) {
				//no task for the target?
				if (target[0] === ':') {
					//if it's abstract, that's an error:
					throw new BuildError(`Target '${target}' (requested by ${from}) is abstract but doesn't have a task.`);
				}
				//otherwise, it's a plain file: add a task that checks it exists:
				const task = async () => {
					try {
						await fsPromises.access(target, fs.constants.R_OK);
					} catch (e) {
						throw new BuildError(`Target '${target}' (requested by ${from}) doesn't exist and doesn't have a task to make it.`);
					}
				};
				task.depends = [];
				task.label = `EXISTS '${target}'`;
				tasks[target] = task;
			}
			if ('neededBy' in tasks[target]) return;
			pending.push(tasks[target]);
			tasks[target].neededBy = [];
			for (let depend of tasks[target].depends) {
				need(depend, `'${target}'`);
				tasks[depend].neededBy.push(tasks[target]);
			}
		}

		//every requested target is needed:
		for (const target of targets) {
			need(target, 'user');
		}

		//----------------------------------
		//now run up to JOBS tasks at once:

		let ready = []; //tasks ready to run
		let running = []; //tasks currently running
		let CANCEL_ALL_TASKS = false; //skip remaining tasks?

		async function launch(task) {
			running.push(task);
			let failedDepends = [];
			for (const depend of task.depends) {
				if (tasks[depend].failed) {
					failedDepends.push(depend);
				} else {
					console.assert(tasks[depend].finished, "all depends should be failed or finished");
				}
			}
			if (failedDepends.length) {
				task.failed = true;
				if (maek.VERBOSE) console.error(`!!! SKIPPED [${task.label}] because target(s) ${failedDepends.join(', ')} failed.`);
			}
			try {
				if (!task.failed) {
					await task();
					task.finished = true;
				}
			} catch (e) {
				if (e instanceof BuildError) {
					console.error(`\x1b[91m!!! FAILED [${task.label}] ${e.message}\x1b[0m`);
					task.failed = true;
					//if -q flag is set, immediately cancel all jobs:
					if (maek.QUIT_EAGERLY) {
						CANCEL_ALL_TASKS = true; //set flag so obs cancel themselves
					}
				} else {
					//don't expect any other exceptions, but if they do arise, re-throw 'em:
					throw e;
				}
			}
			//check all neededBy for potential readiness:
			for (const needed of task.neededBy) {
				let allDone = true;
				for (const depend of needed.depends) {
					if (!(tasks[depend].finished || tasks[depend].failed)) {
						allDone = false;
					}
				}
				if (allDone) {
					ready.push(needed);
				}
			}
			//remove task from 'running' list:
			let i = running.indexOf(task);
			console.assert(i !== -1, "running tasks must exist within running list");
			running.splice(i,1);
		}

		//ready up anything that can be:
		for (const task of pending) {
			if (task.depends.length === 0) {
				ready.push(task);
			}
		}

		//launch tasks until no more can be launched:
		await new Promise((resolve,reject) => {
			function pollTasks() {
				//if can run something now, do so:
				while (running.length < maek.JOBS && !CANCEL_ALL_TASKS && ready.length > 0) {
					launch(ready.shift());
				}
				//if can run something eventually, keep waiting:
				if (running.length > 0 || (!CANCEL_ALL_TASKS && ready.length > 0)) {
					setTimeout(pollTasks, 10);
				} else {
					resolve(); //otherwise, finish
				}
			}
			setImmediate(pollTasks);
		});

		//confirm that nothing was left hanging (dependency loop!):
		let failed = false;
		let skipped = [];
		for (const task of pending) {
			if (!(task.finished || task.failed)) {
				skipped.push(task.label);
			}
			if (!task.finished) {
				failed = true;
			}
		}

		const after = performance.now();
		if (!failed) {
			console.log(` -- SUCCESS: Target(s) '${targets.join("', '")}' brought up to date in ${((after - before) / 1000.0).toFixed(3)} seconds.`);
		} else {
			if (skipped.length) {
				if (CANCEL_ALL_TASKS) {
					console.log(`!!! SKIPPED ${skipped.length} tasks because of failure above.`);
				} else {
					console.log(`\x1b[91m!!! FAILED: tasks ${skipped.join(', ')} were never run (circular dependancy).\x1b[0m`);
				}
			} else {
				console.log(`\x1b[91m!!! FAILED: see error(s) above.\x1b[0m`);
			}
		}

		//store cache to disk:
		saveCache();

		if (maek.VERBOSE) {
			function t(ms) { return (ms / 1000.0).toFixed(3); }
			console.log(`\x1b[35m -- Performance metrics:\x1b[0m`);
			console.log(`\x1b[35m  . hashCache ended up with ${Object.keys(hashCache).length} items and handled ${hashCacheHits} hits.\x1b[0m`);
			console.log(`\x1b[35m  . hashFiles spent ${t(hashLoadTime)} seconds loading and ${t(hashComputeTime)} hashing.\x1b[0m`);
			console.log(`\x1b[35m  . run spent ${t(runTime)} seconds running commands.\x1b[0m`);
		}

		return !failed;
	};


	//automatically call 'update' once the main body of the script has finished running:
	process.nextTick(() => {
		//parse the command line:
		let targets = [];
		for (let argi = 2; argi < process.argv.length; ++argi) {
			const arg = process.argv[argi];
			if (arg === '--') { //-- target target ...
				//the rest of the command line is targets:
				targets.push(...process.argv.slice(argi + 1));
				break;
			} else if (/^-j\d+$/.test(arg)) { //-jN
				//set max jobs
				maek.JOBS = parseInt(arg.substr(2));
			} else if (arg === '-v') {
				//set verbose output
				maek.VERBOSE = true;
			} else if (arg === '-q') {
				//set quit on on first error:
				maek.QUIT_EAGERLY = true;
			} else if (arg.startsWith('-')) { //unrecognized option
				console.error(`Unrecognized option '${arg}'.`);
				process.exit(1);
			} else if (!arg.startsWith('-')) { //a target name
				console.log(`Added target ${arg}.`);
				targets.push(arg);
			}
		}
		if (targets.length !== 0) {
			maek.TARGETS = targets;
		}
		if (maek.TARGETS.length === 0) {
			console.warn("No targets specified on command line and no default targets.");
		}

		maek.update(maek.TARGETS).then((success) => {
			process.exitCode = (success ? 0 : 1);
		});
	});

	return maek;
}
