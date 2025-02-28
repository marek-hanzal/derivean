import type { WithTransaction } from "~/app/db/WithTransaction";
import { withTransportDemand } from "~/app/service/withCycle/withTransport/withTransportDemand";
import { withTransportRoute } from "~/app/service/withCycle/withTransport/withTransportRoute";

export namespace withTransport {
	export interface Props {
		tx: WithTransaction;
		userId: string;
		mapId: string;
	}
}

export const withTransport = async ({ tx, userId, mapId }: withTransport.Props) => {
	await withTransportRoute({ tx, userId, mapId });
	await withTransportDemand({ tx, userId, mapId });
};
