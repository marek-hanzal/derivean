import {BuildingQueryProvider} from "@derivean/building";
import {ConstructionMenu}      from "@derivean/game";
import {
    KingdomConstructionTable,
    withKingdomRepository
}                              from "@derivean/kingdom";
import {container}             from "@derivean/server";
import {IconHammer}            from "@tabler/icons-react";
import {
    HomeIcon,
    Nav,
    Page
}                              from "@use-pico/client";
import {t}                     from "@use-pico/translator";

export namespace List {
    export interface Props {
        params: {
            kingdomId: string;
        };
    }
}

export default async function Available({params: {kingdomId}}: List.Props) {
    const kingdom = await withKingdomRepository.use(container).getOrThrow(kingdomId);

    return <Page
        icon={<IconHammer/>}
        text={{
            header: t`Kingdom building construction (label)`,
        }}
        nav={<Nav
            items={[
                {
                    type: "link",
                    href: {
                        href: "/kingdom/[kingdomId]",
                        query: {kingdomId: kingdom.id},
                    },
                    icon: <HomeIcon/>,
                },
            ]}
        />}
        menu={<ConstructionMenu
            kingdomId={kingdom.id}
            active={["/kingdom/[kingdomId]/building/construction/available"]}
        />}
    >
        <BuildingQueryProvider
            values={{}}
        >
            <KingdomConstructionTable
                kingdomId={kingdom.id}
            />
        </BuildingQueryProvider>
    </Page>;
}
