import {
    HeroQueryStore,
    HeroTable
}                              from "@derivean/hero";
import {withKingdomRepository} from "@derivean/kingdom";
import {container}             from "@derivean/server";
import {HeroIcon}              from "@derivean/ui";
import {t}                     from "@use-pico/i18n";
import {StoreProvider}         from "@use-pico/store";
import {
    Breadcrumbs,
    HomeIcon,
    Page
}                              from "@use-pico/ui";

export namespace List {
    export interface Props {
        params: {
            kingdomId: string;
        };
    }
}

export default async function List({params: {kingdomId}}: List.Props) {
    const kingdom = await withKingdomRepository.use(container).getOrThrow(kingdomId);

    return <Page
        icon={<HeroIcon/>}
        text={{
            header: t()`Kingdom hero list (label)`,
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
    >
        <StoreProvider
            store={HeroQueryStore}
            values={{
                where: {
                    kingdomId: kingdom.id,
                },
            }}
        >
            <HeroTable
                kingdomId={kingdom.id}
            />
        </StoreProvider>
    </Page>;
}
