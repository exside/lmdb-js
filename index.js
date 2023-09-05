import { EventEmitter } from 'events';
import { setExternals, setNativeFunctions, Dbi } from './native.js';
import { arch, tmpdir, platform } from 'os';
import fs from 'fs';
import { Encoder as MsgpackrEncoder } from 'msgpackr';
import { WeakLRUCache } from 'weak-lru-cache';
import * as orderedBinary from 'ordered-binary';


orderedBinary.enableNullTermination();
setExternals({
	arch, fs, tmpdir, MsgpackrEncoder, WeakLRUCache, orderedBinary,
	EventEmitter, os: platform(), onExit(callback) {
		if (process.getMaxListeners() < process.listenerCount('exit') + 8)
			process.setMaxListeners(process.listenerCount('exit') + 8);
		process.on('exit', callback);
	},
});
export { toBufferKey as keyValueToBuffer, compareKeys, compareKeys as compareKey, fromBufferKey as bufferToKeyValue } from 'ordered-binary';
export { ABORT, IF_EXISTS, asBinary } from './write.js';
import { ABORT, IF_EXISTS, asBinary } from './write.js';
export { levelup } from './level.js';
export { SKIP } from './util/RangeIterable.js';
import { levelup } from './level.js';
export { clearKeptObjects } from './native.js';
import { nativeAddon } from './native.js';
export let { noop } = nativeAddon;
export const TIMESTAMP_PLACEHOLDER = new Uint8Array([1,1,1,1,0,0,0,0]);
export const DIRECT_WRITE_PLACEHOLDER = new Uint8Array([1,1,1,2,0,0,0,0]);
export { open, openAsClass, getLastVersion, allDbs, getLastTxnId } from './open.js';
import { toBufferKey as keyValueToBuffer, compareKeys as compareKey, fromBufferKey as bufferToKeyValue } from 'ordered-binary';
import { open, openAsClass, getLastVersion } from './open.js';
export const TransactionFlags = {
	ABORTABLE: 1,
	SYNCHRONOUS_COMMIT: 2,

	NO_SYNC_FLUSH: 0x10000,
};
export default {
	open, openAsClass, getLastVersion, compareKey, keyValueToBuffer, bufferToKeyValue, ABORT, IF_EXISTS, asBinary, levelup, TransactionFlags
};
