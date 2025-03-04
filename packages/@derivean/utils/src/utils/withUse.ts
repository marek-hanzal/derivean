/** @format */

export const withUse = <T extends () => any>(use: { free(): void }, callback: T) => {
	try {
		return callback();
	} finally {
		use.free();
	}
};
