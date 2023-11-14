import {
    type BuildingSchema,
    withConstructionService
}                           from "@derivean/building";
import {container}          from "@derivean/server";
import {
    IconBarrierBlock,
    IconBarrierBlockOff
}                           from "@tabler/icons-react";
import {withDullSchema}     from "@use-pico/dull-stuff";
import {t}                  from "@use-pico/i18n";
import {Alert}              from "@use-pico/ui";
import {type FC}            from "react";
import {type KingdomSchema} from "../schema/KingdomSchema";

export namespace ConstructionDetail {
    export interface Props {
        kingdom: withDullSchema.Infer.Entity<KingdomSchema>;
        building: withDullSchema.Infer.Entity<BuildingSchema>;
    }
}

export const ConstructionDetail: FC<ConstructionDetail.Props> = async (
    {
        kingdom,
        building,
    }
) => {
    const isAvailable = await withConstructionService.use(container).isAvailable(kingdom.inventoryId, building.id);

    return <>
        {isAvailable && <Alert
            icon={<IconBarrierBlock/>}
            color={"green.5"}
            title={t()`Construction is available`}
        >
            {t()`This building is available for construction.`}
        </Alert>}
        {!isAvailable && <Alert
            icon={<IconBarrierBlockOff/>}
            color={"red.5"}
            title={t()`Construction is not available`}
        >
            {t()`You do not have enough resources or workers to build this building. Check requirements.`}
        </Alert>}
    </>;
};
