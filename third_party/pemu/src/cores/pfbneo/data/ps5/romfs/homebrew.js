async function main() {
    const WORKDIR = window.workingDir;
    const PAYLOAD = WORKDIR + '/pfbneo';

    return {
        mainText: "pFBN",
        secondaryText: 'Portable FinalBurn Neo',
	onclick: async () => {
	    return {
		path: PAYLOAD,
                cwd: WORKDIR
	    };
        }
    };
}
