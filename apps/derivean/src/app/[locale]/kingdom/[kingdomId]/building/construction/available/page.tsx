import {
    BuildingQueryStore,
    ConstructionMenu
}                      from "@derivean/building";
import {
    KingdomConstructionTable,
    withKingdomRepository
}                      from "@derivean/kingdom";
import {container}     from "@derivean/server";
import {IconHammer}    from "@tabler/icons-react";
import {t}             from "@use-pico/i18n";
import {StoreProvider} from "@use-pico/store";
import {
    Breadcrumbs,
    HomeIcon,
    Page
}                      from "@use-pico/ui";

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
            header: t()`Kingdom building construction (label)`,
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
            ]}
        />}
        append={<ConstructionMenu
            kingdomId={kingdom.id}
            active={["/kingdom/[kingdomId]/building/construction/available"]}
        />}
    >
        <StoreProvider
            store={BuildingQueryStore}
            values={{}}
        >
            <KingdomConstructionTable
                kingdomId={kingdom.id}
            />
        </StoreProvider>
    </Page>;
}
