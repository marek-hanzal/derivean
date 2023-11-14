import {withBuildingRepository} from "@derivean/building";
import {
    ConstructionDetail,
    withKingdomRepository
}                               from "@derivean/kingdom";
import {container}              from "@derivean/server";
import {BuildingIcon}           from "@derivean/ui";
import {IconHammer}             from "@tabler/icons-react";
import {t}                      from "@use-pico/i18n";
import {
    Breadcrumbs,
    HomeIcon,
    Page
}                               from "@use-pico/ui";

export namespace List {
    export interface Props {
        params: {
            kingdomId: string;
            buildingId: string;
        };
    }
}

export default async function Available(
    {
        params: {
                    kingdomId,
                    buildingId
                }
    }: List.Props
) {
    const kingdom = await withKingdomRepository.use(container).getOrThrow(kingdomId);
    const building = await withBuildingRepository.use(container).getOrThrow(buildingId);

    return <Page
        icon={<BuildingIcon/>}
        text={{
            header: t({values: building})`Kingdom building construction detail (label)`,
        }}
        postfix={<Breadcrumbs
            items={[
                {
                    type: "link",
                    href: {
                        href:  "/kingdom/[kingdomId]",
                        query: {kingdomId: kingdom.id},
                    },
                    icon: <HomeIcon/>,
                },
                {
                    type:  "link",
                    label: t()`Kingdom building construction (label)`,
                    icon:  <IconHammer/>,
                    href:  {
                        href:  "/kingdom/[kingdomId]/building/construction/available",
                        query: {kingdomId: kingdom.id},
                    },
                },
            ]}
        />}
    >
        <ConstructionDetail
            kingdom={kingdom}
            building={building}
        />
    </Page>;
}
